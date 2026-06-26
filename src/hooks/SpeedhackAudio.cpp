#include <Geode/Geode.hpp>
#include <Geode/modify/FMODAudioEngine.hpp>
#include "../Config.hpp"

using namespace geode::prelude;

class $modify(NHSpeedhackAudio, FMODAudioEngine) {
    struct Fields {
        bool applied = false;
    };

    void update(float dt) {
        FMODAudioEngine::update(dt);

        bool active = Config::get().speedhackAudio
                      && Config::get().speedhack
                      && Config::get().speedhackValue > 0.f;

        FMOD::ChannelGroup* masterGroup = nullptr;
        if (m_system->getMasterChannelGroup(&masterGroup) != FMOD_OK) return;

        if (active) {
            masterGroup->setPitch(Config::get().speedhackValue);
            m_fields->applied = true;
        } else if (m_fields->applied) {
            // reset pitch back to normal exactly once when disabled
            masterGroup->setPitch(1.0f);
            m_fields->applied = false;
        }
    }
};
