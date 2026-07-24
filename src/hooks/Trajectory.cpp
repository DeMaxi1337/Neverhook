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
#include <cmath>
#include <unordered_set>

using namespace geode::prelude;

namespace {
    const ccColor4F kHoldColour    = { 0.00f, 1.00f, 0.00f, 1.0f };
    const ccColor4F kReleaseColour = { 0.00f, 0.45f, 0.00f, 1.0f };

    constexpr float kSegmentWidth = 1.0f;
    // GD physics ticks at 240 Hz and PlayerObject::update expects the step in
    // 60fps-relative units, so one real physics tick is 60/240 = 0.25. Stepping
    // at the true tick size (instead of a coarse 0.5) makes the arc accurate
    // and keeps portal transitions smooth instead of kinking.
    constexpr float kIterDelta    = 0.25f; // one 240 Hz physics tick
    constexpr int   kIterCount    = 240;   // ~1 s look-ahead; same per-frame
                                           // cost as the proven build (no lag)
    constexpr float kHitboxOutline = 0.35f;
}

// --- Ghost physics-state clone ---------------------------------------------
// copyAttributes only carries icon/gamemode setup; it leaves the movement
// state (gravity, velocities, dash, slope, ground/jump flags, mode bools)
// stale on the ghost, which made the preview fly up with no gravity and
// invent mid-air jumps. We clone the scalar movement state only -- never
// object/array pointers, since the ghost would then share (and later double-
// free) the live players heap objects on level exit.
static void nhCloneGhostState(PlayerObject* from, PlayerObject* to) {
    to->m_wasTeleported = from->m_wasTeleported;
    to->m_fixGravityBug = from->m_fixGravityBug;
    to->m_reverseSync = from->m_reverseSync;
    to->m_yVelocityBeforeSlope = from->m_yVelocityBeforeSlope;
    to->m_dashX = from->m_dashX;
    to->m_dashY = from->m_dashY;
    to->m_dashAngle = from->m_dashAngle;
    to->m_dashStartTime = from->m_dashStartTime;
    to->m_slopeStartTime = from->m_slopeStartTime;
    to->m_justPlacedStreak = from->m_justPlacedStreak;
    to->m_lastCollisionBottom = from->m_lastCollisionBottom;
    to->m_lastCollisionTop = from->m_lastCollisionTop;
    to->m_lastCollisionLeft = from->m_lastCollisionLeft;
    to->m_lastCollisionRight = from->m_lastCollisionRight;
    to->m_unk50C = from->m_unk50C;
    to->m_unk510 = from->m_unk510;
    to->m_slopeAngle = from->m_slopeAngle;
    to->m_slopeSlidingMaybeRotated = from->m_slopeSlidingMaybeRotated;
    to->m_quickCheckpointMode = from->m_quickCheckpointMode;
    to->m_maybeSavedPlayerFrame = from->m_maybeSavedPlayerFrame;
    to->m_scaleXRelated2 = from->m_scaleXRelated2;
    to->m_groundYVelocity = from->m_groundYVelocity;
    to->m_yVelocityRelated = from->m_yVelocityRelated;
    to->m_scaleXRelated3 = from->m_scaleXRelated3;
    to->m_scaleXRelated4 = from->m_scaleXRelated4;
    to->m_scaleXRelated5 = from->m_scaleXRelated5;
    to->m_isCollidingWithSlope = from->m_isCollidingWithSlope;
    to->m_isBallRotating = from->m_isBallRotating;
    to->m_unk669 = from->m_unk669;
    to->m_collidingWithSlopeId = from->m_collidingWithSlopeId;
    to->m_slopeFlipGravityRelated = from->m_slopeFlipGravityRelated;
    to->m_slopeAngleRadians = from->m_slopeAngleRadians;
    to->m_rotateObjectsRelated = from->m_rotateObjectsRelated;
    to->m_potentialSlopeMap = from->m_potentialSlopeMap;
    to->m_rotationSpeed = from->m_rotationSpeed;
    to->m_rotateSpeed = from->m_rotateSpeed;
    to->m_isRotating = from->m_isRotating;
    to->m_isBallRotating2 = from->m_isBallRotating2;
    to->m_hasGlow = from->m_hasGlow;
    to->m_isHidden = from->m_isHidden;
    to->m_speedMultiplier = from->m_speedMultiplier;
    to->m_yStart = from->m_yStart;
    to->m_gravity = from->m_gravity;
    to->m_trailingParticleLife = from->m_trailingParticleLife;
    to->m_unk648 = from->m_unk648;
    to->m_gameModeChangedTime = from->m_gameModeChangedTime;
    to->m_padRingRelated = from->m_padRingRelated;
    to->m_maybeReducedEffects = from->m_maybeReducedEffects;
    to->m_maybeIsFalling = from->m_maybeIsFalling;
    to->m_shouldTryPlacingCheckpoint = from->m_shouldTryPlacingCheckpoint;
    to->m_playEffects = from->m_playEffects;
    to->m_maybeCanRunIntoBlocks = from->m_maybeCanRunIntoBlocks;
    to->m_hasGroundParticles = from->m_hasGroundParticles;
    to->m_hasShipParticles = from->m_hasShipParticles;
    to->m_isOnGround3 = from->m_isOnGround3;
    to->m_checkpointTimeout = from->m_checkpointTimeout;
    to->m_lastCheckpointTime = from->m_lastCheckpointTime;
    to->m_lastJumpTime = from->m_lastJumpTime;
    to->m_lastFlipTime = from->m_lastFlipTime;
    to->m_flashTime = from->m_flashTime;
    to->m_flashDuration = from->m_flashDuration;
    to->m_flashDelay = from->m_flashDelay;
    to->m_lastSpiderFlipTime = from->m_lastSpiderFlipTime;
    to->m_unkBool5 = from->m_unkBool5;
    to->m_maybeIsVehicleGlowing = from->m_maybeIsVehicleGlowing;
    to->m_switchWaveTrailColor = from->m_switchWaveTrailColor;
    to->m_practiceDeathEffect = from->m_practiceDeathEffect;
    to->m_accelerationOrSpeed = from->m_accelerationOrSpeed;
    to->m_snapDistance = from->m_snapDistance;
    to->m_ringJumpRelated = from->m_ringJumpRelated;
    to->m_ringRelatedSet = from->m_ringRelatedSet;
    to->m_onFlyCheckpointTries = from->m_onFlyCheckpointTries;
    to->m_maybeSpriteRelated = from->m_maybeSpriteRelated;
    to->m_useLandParticles0 = from->m_useLandParticles0;
    to->m_landParticlesAngle = from->m_landParticlesAngle;
    to->m_landParticleRelatedY = from->m_landParticleRelatedY;
    to->m_playerStreak = from->m_playerStreak;
    to->m_streakStrokeWidth = from->m_streakStrokeWidth;
    to->m_disableStreakTint = from->m_disableStreakTint;
    to->m_alwaysShowStreak = from->m_alwaysShowStreak;
    to->m_shipStreakType = from->m_shipStreakType;
    to->m_slopeRotation = from->m_slopeRotation;
    to->m_currentSlopeYVelocity = from->m_currentSlopeYVelocity;
    to->m_unk3d0 = from->m_unk3d0;
    to->m_blackOrbRelated = from->m_blackOrbRelated;
    to->m_unk3e0 = from->m_unk3e0;
    to->m_unk3e1 = from->m_unk3e1;
    to->m_isAccelerating = from->m_isAccelerating;
    to->m_isCurrentSlopeTop = from->m_isCurrentSlopeTop;
    to->m_collidedTopMinY = from->m_collidedTopMinY;
    to->m_collidedBottomMaxY = from->m_collidedBottomMaxY;
    to->m_collidedLeftMaxX = from->m_collidedLeftMaxX;
    to->m_collidedRightMinX = from->m_collidedRightMinX;
    to->m_fadeOutStreak = from->m_fadeOutStreak;
    to->m_canPlaceCheckpoint = from->m_canPlaceCheckpoint;
    to->m_hasCustomGlowColor = from->m_hasCustomGlowColor;
    to->m_maybeIsColliding = from->m_maybeIsColliding;
    to->m_jumpBuffered = from->m_jumpBuffered;
    to->m_stateRingJump = from->m_stateRingJump;
    to->m_wasJumpBuffered = from->m_wasJumpBuffered;
    to->m_wasRobotJump = from->m_wasRobotJump;
    to->m_stateJumpBuffered = from->m_stateJumpBuffered;
    to->m_stateRingJump2 = from->m_stateRingJump2;
    to->m_touchedRing = from->m_touchedRing;
    to->m_touchedCustomRing = from->m_touchedCustomRing;
    to->m_touchedGravityPortal = from->m_touchedGravityPortal;
    to->m_maybeTouchedBreakableBlock = from->m_maybeTouchedBreakableBlock;
    to->m_jumpRelatedAC2 = from->m_jumpRelatedAC2;
    to->m_touchedPad = from->m_touchedPad;
    to->m_yVelocity = from->m_yVelocity;
    to->m_fallSpeed = from->m_fallSpeed;
    to->m_isOnSlope = from->m_isOnSlope;
    to->m_wasOnSlope = from->m_wasOnSlope;
    to->m_slopeVelocity = from->m_slopeVelocity;
    to->m_maybeUpsideDownSlope = from->m_maybeUpsideDownSlope;
    to->m_isShip = from->m_isShip;
    to->m_isBird = from->m_isBird;
    to->m_isBall = from->m_isBall;
    to->m_isDart = from->m_isDart;
    to->m_isRobot = from->m_isRobot;
    to->m_isSpider = from->m_isSpider;
    to->m_isUpsideDown = from->m_isUpsideDown;
    to->m_isDead = from->m_isDead;
    to->m_isOnGround = from->m_isOnGround;
    to->m_isGoingLeft = from->m_isGoingLeft;
    to->m_isSideways = from->m_isSideways;
    to->m_isSwing = from->m_isSwing;
    to->m_reverseRelated = from->m_reverseRelated;
    to->m_maybeReverseSpeed = from->m_maybeReverseSpeed;
    to->m_maybeReverseAcceleration = from->m_maybeReverseAcceleration;
    to->m_xVelocityRelated2 = from->m_xVelocityRelated2;
    to->m_isDashing = from->m_isDashing;
    to->m_dashFireFrame = from->m_dashFireFrame;
    to->m_groundObjectMaterial = from->m_groundObjectMaterial;
    to->m_vehicleSize = from->m_vehicleSize;
    to->m_playerSpeed = from->m_playerSpeed;
    to->m_shipRotation = from->m_shipRotation;
    to->m_lastPortalPos = from->m_lastPortalPos;
    to->m_unkUnused3 = from->m_unkUnused3;
    to->m_isOnGround2 = from->m_isOnGround2;
    to->m_lastLandTime = from->m_lastLandTime;
    to->m_platformerVelocityRelated = from->m_platformerVelocityRelated;
    to->m_maybeIsBoosted = from->m_maybeIsBoosted;
    to->m_scaleXRelatedTime = from->m_scaleXRelatedTime;
    to->m_decreaseBoostSlide = from->m_decreaseBoostSlide;
    to->m_unkA29 = from->m_unkA29;
    to->m_isLocked = from->m_isLocked;
    to->m_controlsDisabled = from->m_controlsDisabled;
    to->m_lastGroundedPos = from->m_lastGroundedPos;
    to->m_touchedRings = from->m_touchedRings;
    to->m_hasEverJumped = from->m_hasEverJumped;
    to->m_hasEverHitRing = from->m_hasEverHitRing;
    to->m_position = from->m_position;
    to->m_isSecondPlayer = from->m_isSecondPlayer;
    to->m_unkA99 = from->m_unkA99;
    to->m_totalTime = from->m_totalTime;
    to->m_isBeingSpawnedByDualPortal = from->m_isBeingSpawnedByDualPortal;
    to->m_audioScale = from->m_audioScale;
    to->m_unkAngle1 = from->m_unkAngle1;
    to->m_yVelocityRelated3 = from->m_yVelocityRelated3;
    to->m_defaultMiniIcon = from->m_defaultMiniIcon;
    to->m_swapColors = from->m_swapColors;
    to->m_switchDashFireColor = from->m_switchDashFireColor;
    to->m_followRelated = from->m_followRelated;
    to->m_playerFollowFloats = from->m_playerFollowFloats;
    to->m_unk838 = from->m_unk838;
    to->m_stateOnGround = from->m_stateOnGround;
    to->m_stateUnk = from->m_stateUnk;
    to->m_stateNoStickX = from->m_stateNoStickX;
    to->m_stateNoStickY = from->m_stateNoStickY;
    to->m_stateUnk2 = from->m_stateUnk2;
    to->m_stateBoostX = from->m_stateBoostX;
    to->m_stateBoostY = from->m_stateBoostY;
    to->m_maybeStateForce2 = from->m_maybeStateForce2;
    to->m_stateScale = from->m_stateScale;
    to->m_platformerXVelocity = from->m_platformerXVelocity;
    to->m_holdingRight = from->m_holdingRight;
    to->m_holdingLeft = from->m_holdingLeft;
    to->m_leftPressedFirst = from->m_leftPressedFirst;
    to->m_scaleXRelated = from->m_scaleXRelated;
    to->m_maybeHasStopped = from->m_maybeHasStopped;
    to->m_xVelocityRelated = from->m_xVelocityRelated;
    to->m_maybeGoingCorrectSlopeDirection = from->m_maybeGoingCorrectSlopeDirection;
    to->m_isSliding = from->m_isSliding;
    to->m_maybeSlopeForce = from->m_maybeSlopeForce;
    to->m_isOnIce = from->m_isOnIce;
    to->m_physDeltaRelated = from->m_physDeltaRelated;
    to->m_isOnGround4 = from->m_isOnGround4;
    to->m_maybeSlidingTime = from->m_maybeSlidingTime;
    to->m_maybeSlidingStartTime = from->m_maybeSlidingStartTime;
    to->m_changedDirectionsTime = from->m_changedDirectionsTime;
    to->m_slopeEndTime = from->m_slopeEndTime;
    to->m_isMoving = from->m_isMoving;
    to->m_platformerMovingLeft = from->m_platformerMovingLeft;
    to->m_platformerMovingRight = from->m_platformerMovingRight;
    to->m_isSlidingRight = from->m_isSlidingRight;
    to->m_maybeChangedDirectionAngle = from->m_maybeChangedDirectionAngle;
    to->m_unkUnused2 = from->m_unkUnused2;
    to->m_isPlatformer = from->m_isPlatformer;
    to->m_stateNoAutoJump = from->m_stateNoAutoJump;
    to->m_stateDartSlide = from->m_stateDartSlide;
    to->m_stateHitHead = from->m_stateHitHead;
    to->m_stateFlipGravity = from->m_stateFlipGravity;
    to->m_gravityMod = from->m_gravityMod;
    to->m_stateForce = from->m_stateForce;
    to->m_stateForceVector = from->m_stateForceVector;
    to->m_affectedByForces = from->m_affectedByForces;
    to->m_jumpPadRelated = from->m_jumpPadRelated;
    to->m_somethingPlayerSpeedTime = from->m_somethingPlayerSpeedTime;
    to->m_playerSpeedAC = from->m_playerSpeedAC;
    to->m_fixRobotJump = from->m_fixRobotJump;
    to->m_holdingButtons = from->m_holdingButtons;
    to->m_inputsLocked = from->m_inputsLocked;
    to->m_gv0123 = from->m_gv0123;
    to->m_iconRequestID = from->m_iconRequestID;
    to->m_unkUnused = from->m_unkUnused;
    to->m_isOutOfBounds = from->m_isOutOfBounds;
    to->m_fallStartY = from->m_fallStartY;
    to->m_disablePlayerSqueeze = from->m_disablePlayerSqueeze;
    to->m_robotAnimation1Enabled = from->m_robotAnimation1Enabled;
    to->m_robotAnimation2Enabled = from->m_robotAnimation2Enabled;
    to->m_spiderAnimationEnabled = from->m_spiderAnimationEnabled;
    to->m_ignoreDamage = from->m_ignoreDamage;
    to->m_enable22Changes = from->m_enable22Changes;
}

class NHTrajectoryNode : public CCDrawNode {
public:
    static NHTrajectoryNode* get() { return s_instance; }
    bool isSimulating() const { return m_simulating; }

    // Orb/pad activation guard: each object may fire at most once per
    // projection run. Without it the held-jump ghost re-triggers the same orb
    // every physics step it overlaps and rockets straight up (the mid-air
    // "jump" garbage). Mirrors Silicate's per-run activated-object tracking.
    bool objectMayFire(cocos2d::CCObject* obj) {
        if (!obj) return true;
        auto key = reinterpret_cast<uintptr_t>(obj);
        if (m_activatedObjects.find(key) != m_activatedObjects.end())
            return false;
        m_activatedObjects.insert(key);
        return true;
    }

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
    int              m_lastFrame  = -1;    // last physics frame we recomputed on
    bool             m_lastDead   = false;
    std::unordered_set<uintptr_t> m_activatedObjects; // orbs/pads fired this run

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
        if (!Config::get().showTrajectory || !m_game || !m_ghost) {
            clear();
            m_lastFrame = -1;
            return;
        }

        // Only recompute when the physics frame actually advanced (or the
        // alive/dead state flipped). Above 240 fps the render loop fires more
        // often than the 240 Hz physics tick, so recomputing the identical
        // projection every render frame was pure wasted cost on big levels.
        int  frame = (int)m_game->m_gameState.m_currentProgress;
        bool dead1 = m_game->m_player1 && m_game->m_player1->m_isDead;
        if (frame == m_lastFrame && dead1 == m_lastDead)
            return;
        m_lastFrame = frame;
        m_lastDead  = dead1;

        clear();

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
        bool liveHolding = live->m_holdingButtons[(int)PlayerButton::Jump];

        // Set up the ghost icon/gamemode, then clone the complete movement
        // state on top so its physics matches the live player exactly.
        m_ghost->copyAttributes(live);
        nhCloneGhostState(live, m_ghost);
        m_ghost->m_maybeReducedEffects = true;
        m_ghost->setPosition(live->m_position);
        m_ghost->setRotation(live->getRotation());
        m_ghost->setVisible(false);
        m_ghost->m_isDead = false;
        m_ghost->m_maybeIsColliding = false;

        // Snapshot the shared game state so the projection cannot leak into
        // the live game; restored right after the loop.
        GJGameState savedGameState = m_game->m_gameState;

        m_simulating = true;
        m_activatedObjects.clear(); // fresh activation set for this run

        m_ghost->releaseButton(PlayerButton::Jump);
        if (held)
            m_ghost->pushButton(PlayerButton::Jump);

        bool matchesLive = (held == liveHolding);
        ccColor4F colour = matchesLive ? kHoldColour : kReleaseColour;

        CCPoint prev = m_ghost->getPosition();
        for (int i = 0; i < kIterCount; ++i) {
            m_ghost->m_collisionLogTop->removeAllObjects();
            m_ghost->m_collisionLogBottom->removeAllObjects();
            m_ghost->m_collisionLogLeft->removeAllObjects();
            m_ghost->m_collisionLogRight->removeAllObjects();

            m_ghost->m_playEffects = false; // no particles/vfx while projecting
            m_ghost->update(kIterDelta);
            m_game->checkCollisions(m_ghost, kIterDelta, false);
            m_ghost->updateRotation(kIterDelta);
            m_ghost->updatePlayerScale();

            CCPoint cur = m_ghost->getPosition();

            // Let the projection flow through portals: mode toggles are applied
            // in customCollisionCheck, so the ghost keeps going in the new
            // gamemode. Only stop when horizontal motion stalls, i.e. the ghost
            // is stuck against geometry and the rest would render as a bogus
            // vertical drop / spike.
            // Only bail when the ghost is fully stuck (no x AND no y motion).
            // A vertical orb launch barely moves in x, so an x-only check would
            // wrongly cut the arc right where the orb fires.
            if (i > 0 && std::fabs(cur.x - prev.x) < 0.05f &&
                         std::fabs(cur.y - prev.y) < 0.05f)
                break;

            drawSegment(prev, cur, kSegmentWidth, colour);
            prev = cur;

            if (m_ghost->m_isDead || m_ghost->m_maybeIsColliding) {
                drawGhostHitbox(colour);
                break;
            }
        }

        m_game->m_gameState = savedGameState;
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
        // player->m_orientedBox can be null for the simulated ghost -> guard it
        // or OBB2D::overlaps dereferences null and crashes (read at 0x144).
        if (obj->m_orientedBox && obj->m_shouldUseOuterOb && player->m_orientedBox)
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
                // Same one-shot guard as orbs so a resting ghost does not get
                // bumped every frame.
                if (!nhTrajSimulating() ||
                    (NHTrajectoryNode::get() && NHTrajectoryNode::get()->objectMayFire(obj))) {
                    if (static_cast<EffectGameObject*>(obj)->m_isReverse)
                        player->reversePlayer(static_cast<EffectGameObject*>(obj));
                    player->bumpPlayer(getBumpMod(player, (int)obj->m_objectType), (int)obj->m_objectType, true, nullptr);
                }
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
                // In simulation, ride each orb only once (see objectMayFire),
                // else a held ghost re-fires it every frame and rockets up.
                if (!nhTrajSimulating() ||
                    (NHTrajectoryNode::get() && NHTrajectoryNode::get()->objectMayFire(obj)))
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
        // Allowed during simulation too: this registers the orbs the ghost
        // flies through so that -- while Jump is held -- it actually rides
        // them. That is what makes the preview predict orb jumps.
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
        // Must run during simulation so the ghost receives the orb's velocity.
        // Only the ghost is updated mid-sim, so the live player stays untouched.
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
