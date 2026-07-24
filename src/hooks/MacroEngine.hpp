#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <filesystem>

// -----------------------------------------------------------------------------
// Neverhook macro engine (TAS-style record / playback).
//
// Frame model: GD 2.2's physics is a fixed-step simulation. The game advances
// GJBaseGameLayer::m_gameState.m_currentProgress by kProgressPerFrame (== 2)
// every physics tick, so the canonical "frame" number is
//     frame = m_gameState.m_currentProgress / kProgressPerFrame
// This is the SAME counter the watermark already uses, so recording and
// playback stay in sync without a custom stepper.
//
// Recording : we hook GJBaseGameLayer::handleButton and store every button
//             event tagged with the frame it happened on.
// Playback  : we hook GJBaseGameLayer::processQueuedButtons and, each tick,
//             re-inject every stored event whose frame has been reached.
// -----------------------------------------------------------------------------

namespace nh {

// One recorded button event.
struct MacroInput {
    uint32_t frame;    // physics frame the input happened on
    uint8_t  button;   // PlayerButton: 1 = Jump, 2 = Left, 3 = Right
    bool     down;     // true = press, false = release
    bool     player1;  // true = P1, false = P2
};

enum class MacroState { Idle, Recording, Playing };

class MacroEngine {
public:
    static MacroEngine& get();

    MacroState state = MacroState::Idle;

    // The action list of the macro currently loaded / being recorded.
    std::vector<MacroInput> inputs;

    // Playback cursor into `inputs`.
    size_t playbackIndex = 0;

    // Playback only actually fires once this becomes true (attempt gate).
    bool started = false;

    // While recording, set when the level resets so the NEXT tick can trim the
    // inputs that belong to the aborted part of the attempt (see recordTick).
    bool pendingRewind = false;

    // Name of the macro currently selected in the UI (no extension).
    std::string currentName = "macro";

    inline bool isRecording() const { return state == MacroState::Recording; }
    inline bool isPlaying()   const { return state == MacroState::Playing;   }
    inline size_t actionCount() const { return inputs.size(); }

    // --- control -------------------------------------------------------------
    void startRecording();   // clears the buffer and arms recording
    void startPlayback();     // arms playback from the start
    void stop();              // back to Idle (keeps the buffer)

    // --- per-tick hooks ------------------------------------------------------
    // Called from GJBaseGameLayer::resetLevel / resetLevelVariables.
    void onLevelReset();
    // Called from handleButton while recording.
    void recordInput(uint32_t frame, int button, bool down, bool player1);
    // Called every tick while recording; handles death / practice-checkpoint
    // rewinds by trimming only the inputs after the frame we rewound to.
    void recordTick(uint32_t frame);

    // Fires every pending input up to (and including) `frame` through `apply`.
    // `apply` signature: void(bool down, int button, bool player1)
    template <class Apply>
    void playbackFrame(uint32_t frame, const Apply& apply) {
        if (state != MacroState::Playing || !started) return;
        while (playbackIndex < inputs.size() &&
               inputs[playbackIndex].frame <= frame) {
            const MacroInput& in = inputs[playbackIndex];
            apply(in.down, static_cast<int>(in.button), in.player1);
            playbackIndex++;
        }
    }

    // --- persistence ---------------------------------------------------------
    std::filesystem::path macrosDir();                       // .../macros
    std::filesystem::path macroPath(const std::string& name); // .../macros/<name>.nhm
    bool save(const std::string& name);
    bool load(const std::string& name);
    std::vector<std::string> listMacros();                   // names without ext
    void openFolder();
};

} // namespace nh
