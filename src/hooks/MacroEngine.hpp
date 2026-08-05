#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <filesystem>

namespace nh {

struct MacroInput {
    uint32_t frame;
    uint8_t  button;
    bool     down;
    bool     player1;
};

enum class MacroState { Idle, Recording, Playing };

class MacroEngine {
public:
    static MacroEngine& get();

    MacroState state = MacroState::Idle;

    std::vector<MacroInput> inputs;

    size_t playbackIndex = 0;

    bool started = false;

    bool pendingRewind = false;

    std::string currentName = "macro";

    inline bool isRecording() const { return state == MacroState::Recording; }
    inline bool isPlaying()   const { return state == MacroState::Playing;   }
    inline size_t actionCount() const { return inputs.size(); }

    void startRecording();
    void startPlayback();
    void stop();

    void onLevelReset();

    void recordInput(uint32_t frame, int button, bool down, bool player1);

    void recordTick(uint32_t frame);

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

    std::filesystem::path macrosDir();
    std::filesystem::path macroPath(const std::string& name);
    bool save(const std::string& name);
    bool load(const std::string& name);
    std::vector<std::string> listMacros();
    void openFolder();
};

}
