#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/CCKeyboardDispatcher.hpp>
#include "../Config.hpp"
#include "FrameAdvanceState.hpp"

using namespace geode::prelude;

namespace {
    // GD advances physics at a fixed 240 ticks per second; one tick == 1/240 s.
    constexpr float kPhysicsTps = 240.0f;
}

class $modify(NHFrameStepper, GJBaseGameLayer) {
    // Must wrap every other update hook so nothing re-runs the tick afterwards.
    static void onModify(auto& self) {
        (void)self.setHookPriorityPre("GJBaseGameLayer::update", Priority::First);
    }

    void update(float dt) {
        auto& c = Config::get();
        if (!c.frameAdvance)
            return GJBaseGameLayer::update(dt);

        bool usable = false;
        if (auto pl = PlayLayer::get())
            usable = !pl->m_isPaused && !pl->m_hasCompletedLevel && pl->m_started
                     && pl->m_player1 && !pl->m_player1->m_isDead;
        else if (auto el = LevelEditorLayer::get())
            usable = el->m_playbackMode == PlaybackMode::Playing;

        if (!usable)
            return GJBaseGameLayer::update(dt);

        bool shouldStep = false;
        if (c.faHold) {
            nh::faHoldDelay += dt;
            bool firstPress = nh::faPressed;
            if (firstPress)
                nh::faHoldDelay = 0.0f;

            if (nh::faDown) {
                shouldStep = firstPress;
                nh::faHoldAdvance++;
                if (!shouldStep && nh::faHoldAdvance >= c.faHoldSpeedCfg) {
                    nh::faHoldAdvance = 0;
                    shouldStep = nh::faHoldDelay >= c.faHoldDelayCfg;
                }
            } else {
                nh::faHoldAdvance = 0;
            }
        } else {
            shouldStep = nh::faPressed;
        }

        nh::faPressed = false;

        // One press -> exactly one physics tick; otherwise freeze the game.
        dt = shouldStep ? (1.0f / kPhysicsTps) : 0.0f;
        GJBaseGameLayer::update(dt);
    }
};

class $modify(NHFrameStepperKeys, cocos2d::CCKeyboardDispatcher) {
    bool dispatchKeyboardMSG(cocos2d::enumKeyCodes key, bool isKeyDown, bool isKeyRepeat, double timestamp) {
        auto& c = Config::get();
        if (nh::faKeyWaiting && isKeyDown && !isKeyRepeat) {
            nh::faKeyWaiting = false;
            c.faStepKey = static_cast<int>(key);
            return true;
        }
        if (c.frameAdvance && key == static_cast<cocos2d::enumKeyCodes>(c.faStepKey)) {
            if (isKeyDown && !isKeyRepeat)
                nh::faPressed = true;
            nh::faDown = isKeyDown;
            return true;
        }
        return CCKeyboardDispatcher::dispatchKeyboardMSG(key, isKeyDown, isKeyRepeat, timestamp);
    }
};
