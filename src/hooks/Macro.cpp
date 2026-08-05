#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include "../Config.hpp"
#include "MacroEngine.hpp"

using namespace geode::prelude;

class $modify(NHMacro, GJBaseGameLayer) {

    uint32_t nhFrame() {
        int p = m_gameState.m_currentProgress;
        if (p < 0) p = 0;
        return static_cast<uint32_t>(p / kProgressPerFrame);
    }

    bool nhLive() {
        auto pl = PlayLayer::get();
        return pl
            && pl->m_started
            && !pl->m_isPaused
            && !pl->m_hasCompletedLevel
            && pl->m_player1
            && !pl->m_player1->m_isDead;
    }

    void handleButton(bool down, int button, bool player1) {
        auto& eng = nh::MacroEngine::get();
        if (eng.isRecording() && nhLive())
            eng.recordInput(nhFrame(), button, down, player1);
        GJBaseGameLayer::handleButton(down, button, player1);
    }

    void processQueuedButtons(float dt, bool clearInputQueue) {
        auto& eng = nh::MacroEngine::get();
        auto& c = Config::get();

        if (eng.isRecording() && eng.pendingRewind)
            eng.recordTick(nhFrame());

        if (eng.isPlaying() && nhLive()) {

            if (!eng.started) {
                if (c.macroPlaybackAttempt <= 0) {
                    eng.started = true;
                } else if (auto pl = PlayLayer::get();
                           pl && pl->m_attempts >= c.macroPlaybackAttempt) {
                    eng.started = true;
                }
            }

            if (eng.started) {
                if (c.macroIgnoreInputs)
                    m_queuedButtons.clear();

                eng.playbackFrame(nhFrame(),
                    [this](bool down, int button, bool player1) {
                        this->handleButton(down, button, player1);
                    });
            }
        }

        GJBaseGameLayer::processQueuedButtons(dt, clearInputQueue);
    }

    void resetLevelVariables() {
        GJBaseGameLayer::resetLevelVariables();
        nh::MacroEngine::get().onLevelReset();
    }
};

class $modify(NHMacroComplete, PlayLayer) {
    void levelComplete() {
        PlayLayer::levelComplete();

        auto& eng = nh::MacroEngine::get();
        auto& c = Config::get();

        if (eng.isRecording()) {
            eng.save(eng.currentName);
            if (c.macroAutoPlayback)
                eng.startPlayback();
            else
                eng.stop();
        } else if (eng.isPlaying()) {
            eng.stop();
        }
    }
};
