#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include "../Config.hpp"

using namespace geode::prelude;

// All Modes Platformer -- lets wave/swing portals trigger in platformer mode.
class $modify(NHAllModesPlatformer, GJBaseGameLayer) {
    void collisionCheckObjects(PlayerObject* p0, gd::vector<GameObject*>* p1, int p2, float p3) {
        GJBaseGameLayer::collisionCheckObjects(p0, p1, p2, p3);

        if (!m_isPlatformer || !Config::get().allModesPlatformer) return;
        if (!p0 || !p1) return;

        for (auto obj : *p1) {
            if (!obj) continue;
            if (obj->m_isDecoration || obj->m_isDecoration2) continue;
            if (obj->m_unk3ee) continue;

            if (auto eff = typeinfo_cast<EffectGameObject*>(obj)) {
                if (obj->m_objectType == GameObjectType::WavePortal
                        || obj->m_objectType == GameObjectType::SwingPortal) {

                    bool hit = obj->m_orientedBox
                        ? obj->m_orientedBox->overlaps(p0->m_orientedBox)
                        : p0->getObjectRect().intersectsRect(obj->getObjectRect());

                    if (hit && this->canBeActivatedByPlayer(p0, eff)) {
                        this->playerWillSwitchMode(p0, obj);
                        this->switchToFlyMode(p0, obj, false, (int)obj->m_objectType);
                        obj->playShineEffect();
                    }
                }
            }
        }
    }
};
