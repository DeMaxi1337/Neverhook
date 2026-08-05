#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include "../Config.hpp"

using namespace geode::prelude;

class $modify(NHInstantComplete, PlayLayer) {
    struct Fields {
        bool fired = false;
    };

    void postUpdate(float dt) {
        PlayLayer::postUpdate(dt);

        if (Config::get().instantComplete && !m_fields->fired && m_started) {
            m_fields->fired = true;
            this->levelComplete();
        }
    }
};
