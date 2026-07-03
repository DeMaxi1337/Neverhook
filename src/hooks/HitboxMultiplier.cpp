#include <Geode/Geode.hpp>
#include <Geode/modify/GameObject.hpp>
#include "../Config.hpp"

using namespace geode::prelude;

// Hitbox Multiplier -- scales player/solid/hazard hitboxes.
class $modify(NHHitboxMultiplier, GameObject) {
    cocos2d::CCRect getObjectRect(float p0, float p1) {
        auto& c = Config::get();
        if (c.hitboxMultiplier) {
            if (typeinfo_cast<PlayerObject*>(this)) {
                p0 *= c.hitboxMultPlayer;
                p1 *= c.hitboxMultPlayer;
            } else if (m_objectType == GameObjectType::Solid
                    || m_objectType == GameObjectType::Slope) {
                p0 *= c.hitboxMultSolid;
                p1 *= c.hitboxMultSolid;
            } else if (m_objectType == GameObjectType::Hazard
                    || m_objectType == GameObjectType::AnimatedHazard) {
                p0 *= c.hitboxMultHazard;
                p1 *= c.hitboxMultHazard;
            }
        }
        return GameObject::getObjectRect(p0, p1);
    }
};
