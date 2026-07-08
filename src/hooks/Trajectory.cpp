#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/LevelEditorLayer.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/PlayerObject.hpp>
#include <Geode/modify/HardStreak.hpp>
#include <Geode/modify/GameObject.hpp>
#include <Geode/modify/EffectGameObject.hpp>

#include "../Config.hpp"

#include <vector>

using namespace geode::prelude;

// Trajectory Prediction -- draws where the player would travel over the next stretch
// of physics ticks, for both the held-jump and released-jump branches. It runs
// a hidden "ghost" PlayerObject through the real collision routine while every
// gameplay side effect is suppressed, so the preview never touches live state.

namespace {
    const ccColor4F kHoldColour    = { 0.00f, 1.00f, 0.00f, 1.0f };
    const ccColor4F kReleaseColour = { 0.00f, 0.45f, 0.00f, 1.0f };

    constexpr float kSegmentWidth = 1.0f;
    constexpr float kIterDelta    = 0.5f; // sub-frame step used by the preview
    constexpr int   kIterCount    = 240;  // how many steps to project forward
    constexpr float kHitboxOutline = 0.35f;

    // Ghost-player state copied from the live player before each projection.
    struct GhostState {
        CCPoint pos;
        float   rotation;
        double  yVelocity;
        float   playerSpeed;
        float   vehicleSize;
        bool    onGround;
        bool    upsideDown;
        bool    dashing;
        bool    isShip, isBall, isBird, isDart, isRobot, isSpider, isSwing;
        bool    holdingJump;
    };

    GhostState captureState(PlayerObject* p) {
        GhostState s;
        s.pos         = p->getPosition();
        s.rotation    = p->getRotation();
        s.yVelocity   = p->m_yVelocity;
        s.playerSpeed = p->m_playerSpeed;
        s.vehicleSize = p->m_vehicleSize;
        s.onGround    = p->m_isOnGround;
        s.upsideDown  = p->m_isUpsideDown;
        s.dashing     = p->m_isDashing;
        s.isShip      = p->m_isShip;
        s.isBall      = p->m_isBall;
        s.isBird      = p->m_isBird;
        s.isDart      = p->m_isDart;
        s.isRobot     = p->m_isRobot;
        s.isSpider    = p->m_isSpider;
        s.isSwing     = p->m_isSwing;
        s.holdingJump = p->m_holdingButtons[(int)PlayerButton::Jump];
        return s;
    }

    void applyState(PlayerObject* p, GhostState const& s) {
        p->setPosition(s.pos);
        p->setRotation(s.rotation);
        p->m_yVelocity   = s.yVelocity;
        p->m_playerSpeed = s.playerSpeed;
        p->m_vehicleSize = s.vehicleSize;
        p->m_isOnGround  = s.onGround;
        p->m_isUpsideDown = s.upsideDown;
        p->m_isDashing   = s.dashing;
        p->m_isShip      = s.isShip;
        p->m_isBall      = s.isBall;
        p->m_isBird      = s.isBird;
        p->m_isDart      = s.isDart;
        p->m_isRobot     = s.isRobot;
        p->m_isSpider    = s.isSpider;
        p->m_isSwing     = s.isSwing;
        p->m_isDead          = false;
        p->m_maybeIsColliding = false;
    }
}

class NHTrajectoryNode : public CCDrawNode {
public:
    static NHTrajectoryNode* get() { return s_instance; }
    bool isSimulating() const { return m_simulating; }

    static NHTrajectoryNode* create(GJBaseGameLayer* game) {
        auto ret = new (std::nothrow) NHTrajectoryNode();
        if (ret && ret->initWithGame(game)) {
            ret->autorelease();
            return ret;
        }
        delete ret;
        return nullptr;
    }

private:
    static inline NHTrajectoryNode* s_instance = nullptr;

    GJBaseGameLayer* m_game  = nullptr;
    PlayerObject*    m_ghost = nullptr;
    bool             m_simulating = false;

    bool initWithGame(GJBaseGameLayer* game) {
        if (!CCDrawNode::init())
            return false;
        if (!game || !game->m_objectLayer)
            return false;

        m_game = game;
        s_instance = this;
        setBlendFunc({ GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA });

        m_ghost = PlayerObject::create(0, 0, game, game->m_objectLayer, false);
        m_ghost->setVisible(false);
        game->m_objectLayer->addChild(m_ghost);

        scheduleUpdate();
        return true;
    }

    ~NHTrajectoryNode() override {
        if (s_instance == this)
            s_instance = nullptr;
    }

    void update(float) override {
        redraw();
    }

    void redraw() {
        clear();

        if (!Config::get().showTrajectory || !m_game || !m_ghost)
            return;

        if (auto p1 = m_game->m_player1; p1 && !p1->m_isDead) {
            simulate(p1, false);
            simulate(p1, true);
        }

        if (auto p2 = m_game->m_player2;
            p2 && m_game->m_gameState.m_isDualMode && !p2->m_isDead) {
            simulate(p2, false);
            simulate(p2, true);
        }
    }

    void simulate(PlayerObject* live, bool held) {
        GhostState state = captureState(live);
        applyState(m_ghost, state);

        m_simulating = true;

        m_ghost->releaseButton(PlayerButton::Jump);
        if (held)
            m_ghost->pushButton(PlayerButton::Jump);

        bool matchesLive = (held == state.holdingJump);
        ccColor4F colour = matchesLive ? kHoldColour : kReleaseColour;

        CCPoint prev = m_ghost->getPosition();
        for (int i = 0; i < kIterCount; ++i) {
            m_ghost->m_collisionLogTop->removeAllObjects();
            m_ghost->m_collisionLogBottom->removeAllObjects();
            m_ghost->m_collisionLogLeft->removeAllObjects();
            m_ghost->m_collisionLogRight->removeAllObjects();

            m_ghost->update(kIterDelta);
            m_game->checkCollisions(m_ghost, kIterDelta, false);
            m_ghost->updateRotation(kIterDelta);
            m_ghost->updatePlayerScale();

            CCPoint cur = m_ghost->getPosition();
            drawSegment(prev, cur, kSegmentWidth, colour);
            prev = cur;

            if (m_ghost->m_isDead || m_ghost->m_maybeIsColliding) {
                drawGhostHitbox(colour);
                break;
            }
        }

        m_simulating = false;
    }

    void drawGhostHitbox(ccColor4F const& colour) {
        ccColor4F hollow = { colour.r, colour.g, colour.b, 0.f };
        auto r = m_ghost->getObjectRect(m_ghost->m_vehicleSize, m_ghost->m_vehicleSize);
        CCPoint quad[4] = {
            ccp(r.getMinX(), r.getMinY()),
            ccp(r.getMaxX(), r.getMinY()),
            ccp(r.getMaxX(), r.getMaxY()),
            ccp(r.getMinX(), r.getMaxY()),
        };
        drawPolygon(quad, 4, hollow, kHitboxOutline, colour);
    }
};

namespace {
    bool nhTrajSimulating() {
        auto node = NHTrajectoryNode::get();
        return node && node->isSimulating();
    }
}

// External accessor: lets other translation units (noclip stats, etc.) tell
// when the trajectory ghost is being simulated so they can ignore its hits.
bool nhTrajectoryIsSimulating() {
    return nhTrajSimulating();
}

// --- Insert the draw node into the play/edit layers -------------------------

class $modify(NHTrajPlayLayer, PlayLayer) {
    struct Fields {
        NHTrajectoryNode* node = nullptr;
    };

    void postUpdate(float dt) {
        PlayLayer::postUpdate(dt);
        if (!m_fields->node && m_objectLayer) {
            m_fields->node = NHTrajectoryNode::create(this);
            if (m_fields->node)
                m_objectLayer->addChild(m_fields->node, 1000);
        }
    }

    void destroyPlayer(PlayerObject* player, GameObject* object) {
        if (nhTrajSimulating() && player) {
            player->m_isDead = true;
            return;
        }
        PlayLayer::destroyPlayer(player, object);
    }

    void playEndAnimationToPos(cocos2d::CCPoint pos) {
        if (!nhTrajSimulating())
            PlayLayer::playEndAnimationToPos(pos);
    }
};

class $modify(NHTrajEditor, LevelEditorLayer) {
    struct Fields {
        NHTrajectoryNode* node = nullptr;
    };

    void postUpdate(float dt) {
        LevelEditorLayer::postUpdate(dt);
        if (!m_fields->node && m_objectLayer) {
            m_fields->node = NHTrajectoryNode::create(this);
            if (m_fields->node)
                m_objectLayer->addChild(m_fields->node, 1000);
        }
    }
};

// --- Suppress every gameplay side effect while the ghost is projecting ------

class $modify(NHTrajBaseLayer, GJBaseGameLayer) {
    void customCollisionCheck(PlayerObject* player, GameObject* obj) {
        bool intersects;
        if (obj->m_orientedBox && obj->m_shouldUseOuterOb)
            intersects = obj->m_orientedBox->overlaps(player->m_orientedBox);
        else
            intersects = player->getObjectRect().intersectsRect(obj->getObjectRect());

        if (!intersects)
            return;

        if (obj->m_objectID == 1829) // S block
            player->stopDashing();

        switch (obj->m_objectType) {
            case GameObjectType::CubePortal:
                player->toggleFlyMode(false, false);
                player->toggleRollMode(false, false);
                player->toggleBirdMode(false, false);
                player->toggleDartMode(false, false);
                player->toggleRobotMode(false, false);
                player->toggleSpiderMode(false, false);
                player->toggleSwingMode(false, false);
                break;
            case GameObjectType::ShipPortal:   player->toggleFlyMode(true, true);    break;
            case GameObjectType::BallPortal:   player->toggleRollMode(true, true);   break;
            case GameObjectType::UfoPortal:    player->toggleBirdMode(true, true);   break;
            case GameObjectType::WavePortal:   player->toggleDartMode(true, true);   break;
            case GameObjectType::RobotPortal:  player->toggleRobotMode(true, true);  break;
            case GameObjectType::SpiderPortal: player->toggleSpiderMode(true, true); break;
            case GameObjectType::SwingPortal:  player->toggleSwingMode(true, true);  break;

            case GameObjectType::NormalGravityPortal:
                this->flipGravity(player, false, true);
                break;
            case GameObjectType::InverseGravityPortal:
                this->flipGravity(player, true, true);
                break;
            case GameObjectType::GravityTogglePortal:
                this->flipGravity(player, !player->m_isUpsideDown, true);
                break;

            case GameObjectType::YellowJumpPad:
            case GameObjectType::PinkJumpPad:
            case GameObjectType::RedJumpPad:
            case GameObjectType::SpiderPad:
                if (static_cast<EffectGameObject*>(obj)->m_isReverse)
                    player->reversePlayer(static_cast<EffectGameObject*>(obj));
                player->bumpPlayer(getBumpMod(player, (int)obj->m_objectType), (int)obj->m_objectType, true, nullptr);
                break;

            case GameObjectType::YellowJumpRing:
            case GameObjectType::PinkJumpRing:
            case GameObjectType::GravityRing:
            case GameObjectType::GreenRing:
            case GameObjectType::DropRing:
            case GameObjectType::RedJumpRing:
            case GameObjectType::CustomRing:
            case GameObjectType::DashRing:
            case GameObjectType::GravityDashRing:
            case GameObjectType::SpiderOrb:
                playerTouchedRing(player, static_cast<RingObject*>(obj));
                break;

            default:
                break;
        }
    }

    void collisionCheckObjects(PlayerObject* p0, gd::vector<GameObject*>* p1, int p2, float p3) {
        if (nhTrajSimulating() && p1) {
            gd::vector<GameObject*> solids = {};
            for (auto obj : *p1) {
                bool passToNative = false;
                if (typeinfo_cast<GameObject*>(obj)) {
                    if (obj->m_objectType == GameObjectType::Solid ||
                        obj->m_objectType == GameObjectType::Hazard ||
                        obj->m_objectType == GameObjectType::AnimatedHazard ||
                        obj->m_objectType == GameObjectType::Slope)
                        passToNative = true;
                }
                if (passToNative)
                    solids.push_back(obj);
                else
                    customCollisionCheck(p0, obj);
            }
            GJBaseGameLayer::collisionCheckObjects(p0, &solids, (int)solids.size(), p3);
            return;
        }
        GJBaseGameLayer::collisionCheckObjects(p0, p1, p2, p3);
    }

    bool canBeActivatedByPlayer(PlayerObject* p0, EffectGameObject* p1) {
        if (nhTrajSimulating())
            return false;
        return GJBaseGameLayer::canBeActivatedByPlayer(p0, p1);
    }

    void playerTouchedRing(PlayerObject* p0, RingObject* p1) {
        if (!nhTrajSimulating())
            GJBaseGameLayer::playerTouchedRing(p0, p1);
    }

    void playerTouchedTrigger(PlayerObject* p0, EffectGameObject* p1) {
        if (!nhTrajSimulating())
            GJBaseGameLayer::playerTouchedTrigger(p0, p1);
    }

    void gameEventTriggered(GJGameEvent p0, int p1, int p2) {
        if (!nhTrajSimulating())
            GJBaseGameLayer::gameEventTriggered(p0, p1, p2);
    }
};

class $modify(NHTrajPlayer, PlayerObject) {
    void incrementJumps() {
        if (!nhTrajSimulating())
            PlayerObject::incrementJumps();
    }

    void playSpiderDashEffect(cocos2d::CCPoint from, cocos2d::CCPoint to) {
        if (!nhTrajSimulating())
            PlayerObject::playSpiderDashEffect(from, to);
    }

    void ringJump(RingObject* ring, bool p1) {
        if (!nhTrajSimulating())
            PlayerObject::ringJump(ring, p1);
    }
};

class $modify(NHTrajStreak, HardStreak) {
    void addPoint(cocos2d::CCPoint pt) {
        if (!nhTrajSimulating())
            HardStreak::addPoint(pt);
    }
};

class $modify(NHTrajObject, GameObject) {
    void playShineEffect() {
        if (!nhTrajSimulating())
            GameObject::playShineEffect();
    }
};

class $modify(NHTrajEffect, EffectGameObject) {
    void triggerObject(GJBaseGameLayer* p0, int p1, const gd::vector<int>* p2) {
        if (!nhTrajSimulating())
            EffectGameObject::triggerObject(p0, p1, p2);
    }
};
