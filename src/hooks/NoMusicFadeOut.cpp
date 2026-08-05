#include <Geode/Geode.hpp>
#include <Geode/modify/FMODAudioEngine.hpp>
#include "../Config.hpp"

using namespace geode::prelude;

class $modify(NHNoMusicFadeOut, FMODAudioEngine) {
    void fadeOutMusic(float duration, int channel) {
        if (Config::get().noMusicFadeOut)
            return;
        FMODAudioEngine::fadeOutMusic(duration, channel);
    }
};
