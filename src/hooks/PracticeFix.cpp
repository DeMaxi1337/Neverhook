#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include "../Config.hpp"
#include "MacroEngine.hpp"

using namespace geode::prelude;

// Practice fix is active when the user enabled it OR while a macro is being
// recorded, so checkpoint restores stay frame-accurate during recording.
static bool nhPracticeFixActive() {
    return Config::get().practiceFix || nh::MacroEngine::get().isRecording();
}

struct NHPlayerSnap {
    cocos2d::CCPoint pos;
    float            rot;
    double           velY;
    bool             onGround;
    bool             upsideDown;
    bool             dashing;
    float            playerSpeed;
};

struct NHCheckpointSnap {
    NHPlayerSnap p1;
    NHPlayerSnap p2;
    bool         hasP2 = false;
};

static NHPlayerSnap nhSnapPlayer(PlayerObject* p) {
    NHPlayerSnap s;
    s.pos         = p->getPosition();
    s.rot         = p->getRotation();
    s.velY        = p->m_yVelocity;
    s.onGround    = p->m_isOnGround;
    s.upsideDown  = p->m_isUpsideDown;
    s.dashing     = p->m_isDashing;
    s.playerSpeed = p->m_playerSpeed;
    return s;
}

static void nhApplyPlayer(PlayerObject* p, const NHPlayerSnap& s) {
    p->setPosition(s.pos);
    p->setRotation(s.rot);
    p->m_yVelocity   = s.velY;
    p->m_isOnGround  = s.onGround;
    p->m_isUpsideDown = s.upsideDown;
    p->m_isDashing   = s.dashing;
    p->m_playerSpeed = s.playerSpeed;
}

class $modify(NHPracticeFix, PlayLayer) {
    struct Fields {
        std::unordered_map<CheckpointObject*, NHCheckpointSnap> snaps;
    };

    void resetLevel() {
        if (m_checkpointArray->count() == 0)
            m_fields->snaps.clear();
        PlayLayer::resetLevel();
    }

    CheckpointObject* createCheckpoint() {
        auto cp = PlayLayer::createCheckpoint();
        if (!cp || !nhPracticeFixActive())
            return cp;
        if (m_gameState.m_currentProgress <= 0)
            return cp;

        NHCheckpointSnap snap;
        snap.p1    = nhSnapPlayer(m_player1);
        snap.hasP2 = m_gameState.m_isDualMode && m_player2;
        if (snap.hasP2)
            snap.p2 = nhSnapPlayer(m_player2);
        m_fields->snaps[cp] = std::move(snap);
        return cp;
    }

    void loadFromCheckpoint(CheckpointObject* cp) {
        PlayLayer::loadFromCheckpoint(cp);
        if (!nhPracticeFixActive())
            return;
        auto it = m_fields->snaps.find(cp);
        if (it == m_fields->snaps.end())
            return;
        const auto& snap = it->second;
        nhApplyPlayer(m_player1, snap.p1);
        if (snap.hasP2 && m_player2)
            nhApplyPlayer(m_player2, snap.p2);
    }

    void removeCheckpoint(bool first) {
        if (m_checkpointArray->count() > 0) {
            auto* cp = first
                ? static_cast<CheckpointObject*>(m_checkpointArray->objectAtIndex(0))
                : static_cast<CheckpointObject*>(m_checkpointArray->lastObject());
            m_fields->snaps.erase(cp);
        }
        PlayLayer::removeCheckpoint(first);
    }

#ifdef GEODE_IS_DESKTOP
    void storeCheckpoint(CheckpointObject* cp) {
        auto* old = m_checkpointArray->count() > 0
            ? static_cast<CheckpointObject*>(m_checkpointArray->objectAtIndex(0))
            : nullptr;
        PlayLayer::storeCheckpoint(cp);
        if (old && !m_checkpointArray->containsObject(old))
            m_fields->snaps.erase(old);
    }
#endif
};
