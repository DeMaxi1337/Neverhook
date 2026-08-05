#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/LevelEditorLayer.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/PlayerObject.hpp>
#include <Geode/modify/HardStreak.hpp>
#include <Geode/modify/GameObject.hpp>
#include <Geode/modify/EffectGameObject.hpp>

#include "../Config.hpp"

using namespace geode::prelude;

namespace {

constexpr float kStepDelta   = 0.25f;
constexpr int   kMaxSteps    = 320;
constexpr float kLineWidth   = 0.55f;
constexpr float kHitboxWidth = 0.32f;

enum TrajMode {
    ModeHold = 0,
    ModeSwift,
    ModeRelease,
    ModeCount
};

const ccColor4F kColours[ModeCount] = {
    { 0.20f, 1.00f, 0.30f, 1.00f },
    { 1.00f, 0.95f, 0.25f, 0.95f },
    { 1.00f, 0.25f, 0.25f, 1.00f },
};

ccColor4F invertColour(ccColor4F const& in) {
    return { 1.0f - in.r, 1.0f - in.g, 1.0f - in.b, in.a };
}

class TrajectoryDrawNode : public CCDrawNode {
public:
    static TrajectoryDrawNode* create() {
        auto* node = new TrajectoryDrawNode();
        if (node->init()) {
            node->autorelease();
            node->m_bUseArea = false;
            return node;
        }
        delete node;
        return nullptr;
    }
};

struct Signature {
    float    values[20] = {};
    uint64_t flags1     = 0;
    uint64_t flags2     = 0;
    uint32_t bits       = 0;

    bool equals(Signature const& other) const {
        if (flags1 != other.flags1) return false;
        if (flags2 != other.flags2) return false;
        if (bits != other.bits) return false;
        for (int i = 0; i < 20; i++) {
            if (values[i] != other.values[i]) return false;
        }
        return true;
    }
};

uint64_t packFlags(PlayerObject* player) {
    uint64_t flags = 0;
    int bit = 0;
    auto set = [&](bool value) {
        if (value) flags |= (uint64_t(1) << bit);
        bit++;
    };
    set(player->m_isShip);
    set(player->m_isBird);
    set(player->m_isDart);
    set(player->m_isSwing);
    set(player->m_isBall);
    set(player->m_isSpider);
    set(player->m_isRobot);
    set(player->m_isUpsideDown);
    set(player->m_isDead);
    set(player->m_isOnGround);
    set(player->m_isDashing);
    set(player->m_isSideways);
    set(player->m_jumpBuffered);
    set(player->m_isOnSlope);
    set(player->m_holdingButtons[static_cast<int>(PlayerButton::Jump)]);
    set(player->m_holdingButtons[static_cast<int>(PlayerButton::Left)]);
    set(player->m_holdingButtons[static_cast<int>(PlayerButton::Right)]);
    return flags;
}

class Simulation {
public:
    static Simulation& get() {
        static Simulation instance;
        return instance;
    }

    bool drawing() const { return m_drawing; }

    bool isGhost(PlayerObject* player) const {
        return player && (player == m_ghost1 || player == m_ghost2);
    }

    void reportDeath(PlayerObject* player) {
        if (player == m_ghost1) m_dead1 = true;
        if (player == m_ghost2) m_dead2 = true;
    }

    void detach() {
        releaseGhost(m_ghost1);
        releaseGhost(m_ghost2);
        m_ghost1 = nullptr;
        m_ghost2 = nullptr;

        if (m_draw) {
            if (m_draw->getParent()) m_draw->removeFromParent();
            m_draw->release();
            m_draw = nullptr;
        }

        m_game      = nullptr;
        m_hasCache  = false;
        m_drawing   = false;
    }

    void tick(GJBaseGameLayer* game) {
        if (!game) return;

        if (!Config::get().showTrajectory) {
            if (m_draw) m_draw->setVisible(false);
            return;
        }

        if (game != m_game) {
            detach();
            attach(game);
        }

        if (!m_ghost1 || !m_draw) return;

        PlayerObject* real1 = game->m_player1;
        if (!real1) return;

        m_draw->setVisible(true);

        Signature current = computeSignature(game);
        if (m_hasCache && current.equals(m_signature)) return;

        m_signature = current;
        m_hasCache  = true;

        m_draw->clear();
        m_drawing = true;

        const bool dual = game->m_gameState.m_isDualMode && game->m_player2 != nullptr;

        for (int mode = 0; mode < ModeCount; mode++) {
            simulate(game, true, mode);
            if (dual) simulate(game, false, mode);
        }

        m_drawing = false;

        if (m_ghost1) m_ghost1->setVisible(false);
        if (m_ghost2) m_ghost2->setVisible(false);
    }

private:
    GJBaseGameLayer*    m_game     = nullptr;
    PlayerObject*       m_ghost1   = nullptr;
    PlayerObject*       m_ghost2   = nullptr;
    TrajectoryDrawNode* m_draw     = nullptr;
    bool                m_drawing  = false;
    bool                m_dead1    = false;
    bool                m_dead2    = false;
    bool                m_hasCache = false;
    Signature           m_signature;

    void releaseGhost(PlayerObject* ghost) {
        if (!ghost) return;
        if (ghost->getParent()) ghost->removeFromParent();
        ghost->release();
    }

    CCNode* hostFor(GJBaseGameLayer* game) {
        CCNode* host = nullptr;
        if (game->m_debugDrawNode) host = game->m_debugDrawNode->getParent();
        if (!host) host = game->m_objectLayer;
        if (!host) host = game;
        if (typeinfo_cast<CCSpriteBatchNode*>(host)) host = game;
        return host;
    }

    PlayerObject* spawnGhost(GJBaseGameLayer* game, char const* id) {
        PlayerObject* ghost = PlayerObject::create(1, 1, game, game, true);
        if (!ghost) return nullptr;

        ghost->retain();
        ghost->setID(id);
        ghost->setPosition({ 0.f, 105.f });
        ghost->setVisible(false);

        CCNode* host = game->m_objectLayer;
        if (host && !typeinfo_cast<CCSpriteBatchNode*>(host)) host->addChild(ghost);

        return ghost;
    }

    void attach(GJBaseGameLayer* game) {
        m_game   = game;
        m_ghost1 = spawnGhost(game, "nh-trajectory-ghost-1");
        m_ghost2 = spawnGhost(game, "nh-trajectory-ghost-2");

        m_draw = TrajectoryDrawNode::create();
        if (!m_draw) return;

        m_draw->retain();
        m_draw->setID("nh-trajectory-draw");
        m_draw->setBlendFunc(ccBlendFunc{ GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA });

        CCNode* host = hostFor(game);
        if (host) host->addChild(m_draw, 900000);
    }

    Signature computeSignature(GJBaseGameLayer* game) {
        Signature sig;

        if (PlayerObject* player = game->m_player1) {
            const CCPoint pos = player->getPosition();
            sig.values[0] = pos.x;
            sig.values[1] = pos.y;
            sig.values[2] = player->getRotation();
            sig.values[3] = player->m_yVelocity;
            sig.values[4] = player->m_gravityMod;
            sig.values[5] = player->m_vehicleSize;
            sig.values[6] = player->m_speedMultiplier;
            sig.flags1    = packFlags(player);
        }

        if (PlayerObject* player = game->m_player2) {
            const CCPoint pos = player->getPosition();
            sig.values[7]  = pos.x;
            sig.values[8]  = pos.y;
            sig.values[9]  = player->getRotation();
            sig.values[10] = player->m_yVelocity;
            sig.values[11] = player->m_gravityMod;
            sig.values[12] = player->m_vehicleSize;
            sig.values[13] = player->m_speedMultiplier;
            sig.flags2     = packFlags(player);
        }

        sig.values[14] = game->m_gameState.m_timeWarp;
        sig.values[15] = game->m_gameState.m_currentProgress;

        uint32_t bits = 0;
        int bit = 0;
        auto set = [&](bool value) {
            if (value) bits |= (uint32_t(1) << bit);
            bit++;
        };
        set(game->m_gameState.m_isDualMode);
        set(game->m_player1 != nullptr);
        set(game->m_player2 != nullptr);
        sig.bits = bits;

        return sig;
    }

    void clearCollisionLog(PlayerObject* ghost) {
        if (ghost->m_collisionLogTop)    ghost->m_collisionLogTop->removeAllObjects();
        if (ghost->m_collisionLogBottom) ghost->m_collisionLogBottom->removeAllObjects();
        if (ghost->m_collisionLogLeft)   ghost->m_collisionLogLeft->removeAllObjects();
        if (ghost->m_collisionLogRight)  ghost->m_collisionLogRight->removeAllObjects();
    }

    void copyState(PlayerObject* real, PlayerObject* ghost) {
        ghost->copyAttributes(real);

        ghost->m_yVelocity            = real->m_yVelocity;
        ghost->m_gravity              = real->m_gravity;
        ghost->m_gravityMod           = real->m_gravityMod;
        ghost->m_fallSpeed            = real->m_fallSpeed;
        ghost->m_speedMultiplier      = real->m_speedMultiplier;
        ghost->m_isOnGround           = real->m_isOnGround;
        ghost->m_isOnSlope            = real->m_isOnSlope;
        ghost->m_wasOnSlope           = real->m_wasOnSlope;
        ghost->m_slopeVelocity        = real->m_slopeVelocity;
        ghost->m_slopeAngle           = real->m_slopeAngle;
        ghost->m_slopeStartTime       = real->m_slopeStartTime;
        ghost->m_isCollidingWithSlope = real->m_isCollidingWithSlope;
        ghost->m_yVelocityBeforeSlope = real->m_yVelocityBeforeSlope;
        ghost->m_lastCollisionTop     = real->m_lastCollisionTop;
        ghost->m_lastCollisionBottom  = real->m_lastCollisionBottom;
        ghost->m_lastCollisionLeft    = real->m_lastCollisionLeft;
        ghost->m_lastCollisionRight   = real->m_lastCollisionRight;
        ghost->m_dashX                = real->m_dashX;
        ghost->m_dashY                = real->m_dashY;
        ghost->m_dashAngle            = real->m_dashAngle;
        ghost->m_dashStartTime        = real->m_dashStartTime;
        ghost->m_jumpBuffered         = real->m_jumpBuffered;
        ghost->m_stateRingJump        = real->m_stateRingJump;
        ghost->m_touchedRing          = real->m_touchedRing;
        ghost->m_touchedPad           = real->m_touchedPad;
        ghost->m_wasTeleported        = real->m_wasTeleported;
        ghost->m_fixGravityBug        = real->m_fixGravityBug;
        ghost->m_isDead               = false;

        for (int i = 0; i < 3; i++) {
            ghost->m_holdingButtons[i] = real->m_holdingButtons[i];
        }

        ghost->setPosition(real->getPosition());
        ghost->setRotation(real->getRotation());
        ghost->setScaleX(real->getScaleX());
        ghost->setScaleY(real->getScaleY());
        ghost->setVisible(false);

        clearCollisionLog(ghost);
    }

    void applyMode(PlayerObject* ghost, int mode) {
        switch (mode) {
            case ModeHold: {
                ghost->pushButton(PlayerButton::Jump);
                break;
            }
            case ModeSwift: {
                ghost->pushButton(PlayerButton::Jump);
                ghost->releaseButton(PlayerButton::Jump);
                break;
            }
            default: {
                ghost->releaseButton(PlayerButton::Jump);
                ghost->m_jumpBuffered = false;
                break;
            }
        }
    }

    void simulate(GJBaseGameLayer* game, bool first, int mode) {
        PlayerObject* ghost = first ? m_ghost1 : m_ghost2;
        PlayerObject* real  = first ? game->m_player1 : game->m_player2;
        if (!ghost || !real || real->m_isDead) return;

        GJGameState savedState = game->m_gameState;

        EffectManagerState savedEffects;
        bool hasEffects = game->m_effectManager != nullptr;
        if (hasEffects) game->m_effectManager->saveToState(savedEffects);

        copyState(real, ghost);
        applyMode(ghost, mode);

        if (first) m_dead1 = false;
        else       m_dead2 = false;

        ccColor4F colour = kColours[mode];
        if (!first) colour = invertColour(colour);

        const float zoom  = game->m_gameState.m_cameraZoom > 0.05f
                                ? game->m_gameState.m_cameraZoom
                                : 1.0f;
        const float width = kLineWidth / zoom;

        int steps = 0;
        for (int i = 0; i < kMaxSteps; i++) {
            if (!step(game, ghost, colour, width)) break;
            steps++;
            if (first ? m_dead1 : m_dead2) break;
        }

        if (steps > 1) drawHitbox(ghost, colour, width);

        game->m_gameState = savedState;
        if (hasEffects) game->m_effectManager->loadFromState(savedEffects);

        ghost->setVisible(false);
    }

    bool step(GJBaseGameLayer* game, PlayerObject* ghost, ccColor4F const& colour, float width) {
        const CCPoint from = ghost->getPosition();

        game->m_gameState.m_currentProgress++;

        clearCollisionLog(ghost);

        ghost->m_playEffects = false;
        ghost->update(kStepDelta);
        ghost->updateRotation(kStepDelta);

        if (game->checkCollisions(ghost, kStepDelta, false) == 1) {
            reportDeath(ghost);
        }

        if (game->m_effectManager) game->m_effectManager->postCollisionCheck();

        m_draw->drawSegment(from, ghost->getPosition(), width, colour);

        return true;
    }

    void drawHitbox(PlayerObject* ghost, ccColor4F const& colour, float width) {
        CCRect rect = ghost->getObjectRect();

        CCPoint points[4] = {
            { rect.getMinX(), rect.getMinY() },
            { rect.getMaxX(), rect.getMinY() },
            { rect.getMaxX(), rect.getMaxY() },
            { rect.getMinX(), rect.getMaxY() },
        };

        const float angle = ghost->getRotation();
        const CCPoint mid = { rect.getMidX(), rect.getMidY() };
        for (int i = 0; i < 4; i++) {
            points[i] = points[i].rotateByAngle(mid, -CC_DEGREES_TO_RADIANS(angle));
        }

        m_draw->drawPolygon(points, 4, { 0.f, 0.f, 0.f, 0.f },
                            width * (kHitboxWidth / kLineWidth), colour);
    }
};

}

bool nhTrajectoryIsSimulating() {
    return Simulation::get().drawing();
}

class $modify(NHTrajPlayLayer, PlayLayer) {
    void postUpdate(float dt) {
        PlayLayer::postUpdate(dt);
        Simulation::get().tick(this);
    }

    void destroyPlayer(PlayerObject* player, GameObject* object) {
        if (Simulation::get().drawing()) {
            Simulation::get().reportDeath(player);
            return;
        }
        PlayLayer::destroyPlayer(player, object);
    }

    void levelComplete() {
        if (Simulation::get().drawing()) return;
        PlayLayer::levelComplete();
    }

    void playEndAnimationToPos(cocos2d::CCPoint pos) {
        if (Simulation::get().drawing()) return;
        PlayLayer::playEndAnimationToPos(pos);
    }

    void storeCheckpoint(CheckpointObject* checkpoint) {
        if (Simulation::get().drawing()) return;
        PlayLayer::storeCheckpoint(checkpoint);
    }

    void onQuit() {
        Simulation::get().detach();
        PlayLayer::onQuit();
    }
};

class $modify(NHTrajEditor, LevelEditorLayer) {
    void postUpdate(float dt) {
        LevelEditorLayer::postUpdate(dt);
        if (m_playbackMode == PlaybackMode::Playing) Simulation::get().tick(this);
    }
};

class $modify(NHTrajBaseLayer, GJBaseGameLayer) {
    void playerTouchedRing(PlayerObject* player, RingObject* ring) {
        if (Simulation::get().drawing()) {
            if (Simulation::get().isGhost(player)) player->ringJump(ring, false);
            return;
        }
        GJBaseGameLayer::playerTouchedRing(player, ring);
    }

    void playerTouchedTrigger(PlayerObject* player, EffectGameObject* object) {
        if (Simulation::get().drawing()) return;
        GJBaseGameLayer::playerTouchedTrigger(player, object);
    }

    void gameEventTriggered(GJGameEvent event, int p1, int p2) {
        if (Simulation::get().drawing()) return;
        GJBaseGameLayer::gameEventTriggered(event, p1, p2);
    }

    void shakeCamera(float duration, float strength, float interval) {
        if (Simulation::get().drawing()) return;
        GJBaseGameLayer::shakeCamera(duration, strength, interval);
    }
};

class $modify(NHTrajPlayer, PlayerObject) {
    void incrementJumps() {
        if (Simulation::get().drawing()) return;
        PlayerObject::incrementJumps();
    }

    void playSpiderDashEffect(cocos2d::CCPoint from, cocos2d::CCPoint to) {
        if (Simulation::get().drawing()) return;
        PlayerObject::playSpiderDashEffect(from, to);
    }

    void playDeathEffect() {
        if (Simulation::get().drawing()) return;
        PlayerObject::playDeathEffect();
    }

    void playSpawnEffect() {
        if (Simulation::get().drawing()) return;
        PlayerObject::playSpawnEffect();
    }
};

class $modify(NHTrajStreak, HardStreak) {
    void addPoint(cocos2d::CCPoint point) {
        if (Simulation::get().drawing()) return;
        HardStreak::addPoint(point);
    }
};

class $modify(NHTrajObject, GameObject) {
    void playShineEffect() {
        if (Simulation::get().drawing()) return;
        GameObject::playShineEffect();
    }
};

class $modify(NHTrajEffect, EffectGameObject) {
    void triggerObject(GJBaseGameLayer* layer, int p1, const gd::vector<int>* p2) {
        if (Simulation::get().drawing()) return;
        EffectGameObject::triggerObject(layer, p1, p2);
    }
};
