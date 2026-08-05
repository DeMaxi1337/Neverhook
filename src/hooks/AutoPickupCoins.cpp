#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/binding/EffectGameObject.hpp>
#include "../Config.hpp"

#include <vector>

using namespace geode::prelude;

class $modify(NHAutoPickupCoins, PlayLayer) {
    struct Fields {
        std::vector<EffectGameObject*> coins;
    };

    void addObject(GameObject* obj) {
        PlayLayer::addObject(obj);
        if (!obj) return;
        int id = obj->m_objectID;
        if (id == 142 || id == 1329)
            m_fields->coins.push_back(static_cast<EffectGameObject*>(obj));
    }

    void resetLevel() {
        PlayLayer::resetLevel();
        if (!Config::get().autoPickupCoins) return;
        for (auto* coin : m_fields->coins) {
            if (!coin) continue;
            this->pickupItem(coin);
            this->destroyObject(coin);
        }
    }
};
