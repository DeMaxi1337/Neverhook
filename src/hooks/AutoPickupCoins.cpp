#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/binding/EffectGameObject.hpp>
#include "../Config.hpp"

#include <vector>

using namespace geode::prelude;

class $modify(NHAutoPickupCoins, PlayLayer) {
    void resetLevel() {
        PlayLayer::resetLevel();
        if (!Config::get().autoPickupCoins || !m_objects) return;
        for (auto obj : CCArrayExt<GameObject*>(m_objects)) {
            if (!obj) continue;
            int id = obj->m_objectID;
            if (id == 142 || id == 1329) {
                auto coin = static_cast<EffectGameObject*>(obj);
                this->pickupItem(coin);
                this->destroyObject(coin);
            }
        }
    }
};
