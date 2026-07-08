#include <Geode/Geode.hpp>
#include <Geode/modify/FMODAudioEngine.hpp>
#include "../Config.hpp"

using namespace geode::prelude;

// Skips the music fade-out so the song cuts cleanly (e.g. on death/complete).
class $modify(NHNoMusicFadeOut, FMODAudioEngine) {
    void fadeOutMusic(float duration, int channel) {
        if (Config::get().noMusicFadeOut)
            return;
        FMODAudioEngine::fadeOutMusic(duration, channel);
    }
};
