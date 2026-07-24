#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include "../Config.hpp"
#include "MacroEngine.hpp"

using namespace geode::prelude;

// Macro record / playback hooks.
//
// Everything is gated behind nhLive(): we only ever record or inject during a
// real, RUNNING attempt with a LIVE player. Touching input on the death /
// reset frame (player mid-destruction) is what crashed the game on death,
// restart and playtest, so the m_isDead guard below is mandatory. This mirrors
// the guard already used in FrameAdvance.cpp.
class $modify(NHMacro, GJBaseGameLayer) {

    // Canonical physics frame (see MacroEngine.hpp). Same counter the
    // watermark uses: m_currentProgress advances by kProgressPerFrame per tick.
    uint32_t nhFrame() {
        int p = m_gameState.m_currentProgress;
        if (p < 0) p = 0;
        return static_cast<uint32_t>(p / kProgressPerFrame);
    }

    // Only true when it is SAFE to record/inject input.
    bool nhLive() {
        auto pl = PlayLayer::get();
        return pl
            && pl->m_started
            && !pl->m_isPaused
            && !pl->m_hasCompletedLevel
            && pl->m_player1
            && !pl->m_player1->m_isDead;
    }

    // Recording: capture the real button event, tagged with its frame.
    void handleButton(bool down, int button, bool player1) {
        auto& eng = nh::MacroEngine::get();
        if (eng.isRecording() && nhLive())
            eng.recordInput(nhFrame(), button, down, player1);
        GJBaseGameLayer::handleButton(down, button, player1);
    }

    // Playback: re-inject the stored inputs scheduled for this frame.
    void processQueuedButtons(float dt, bool clearInputQueue) {
        auto& eng = nh::MacroEngine::get();
        auto& c = Config::get();

        // Recording: after a death / checkpoint rewind, trim the inputs from
        // the aborted part. Only compute the frame on the reset tick (when
        // pendingRewind is set) so nothing runs on normal recording ticks.
        if (eng.isRecording() && eng.pendingRewind)
            eng.recordTick(nhFrame());

        if (eng.isPlaying() && nhLive()) {
            // Attempt gate. 0 (or less) => playback starts immediately; any
            // other value waits until that attempt begins (at its own frame 0,
            // so we never dump the whole macro mid-run).
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

    // Every attempt reset rewinds playback / restarts recording.
    void resetLevelVariables() {
        GJBaseGameLayer::resetLevelVariables();
        nh::MacroEngine::get().onLevelReset();
    }
};

// Stop the macro by itself once the level is completed (incl. practice), and
// finalize a recording. If "auto playback" is on, a fresh recording flips
// straight into playback so the next attempt replays it.
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
