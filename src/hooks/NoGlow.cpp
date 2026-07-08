#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/GameObject.hpp>
#include "../Config.hpp"

using namespace geode::prelude;

class $modify(NHNoGlow, PlayLayer) {
    void addObject(GameObject* object) {
        if (Config::get().noGlow && object)
            object->m_hasNoGlow = true;
        PlayLayer::addObject(object);
    }
};
