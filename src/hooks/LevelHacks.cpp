#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/GameObject.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include "../Config.hpp"

using namespace geode::prelude;

class $modify(NHJumpHack, GJBaseGameLayer) {
    void update(float dt) {
        if (Config::get().jumpHack) {
            if (m_player1) m_player1->m_isOnGround = true;
            if (m_player2) m_player2->m_isOnGround = true;
        }
        GJBaseGameLayer::update(dt);
    }
};

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

class $modify(NHAutoclicker, GJBaseGameLayer) {
    struct Fields {
        double timeP1  = 0.0;
        double timeP2  = 0.0;
        bool   stateP1 = false;
        bool   stateP2 = false;
    };

    void processQueuedButtons(float dt, bool clearInputQueue) {
        auto& c = Config::get();

        auto pl = PlayLayer::get();
        bool paused = pl && pl->m_isPaused;

        if (c.autoclicker && !paused && c.autoclickerCps > 0.f) {
            float interval = 1.0f / c.autoclickerCps / 2.0f;
            m_fields->timeP1 += dt;
            while (m_fields->timeP1 >= interval) {
                m_fields->stateP1 = !m_fields->stateP1;
                m_fields->timeP1 -= interval;
                this->handleButton(m_fields->stateP1, (int)PlayerButton::Jump, true);
            }
        }

        if (c.autoclickerP2 && !paused && c.autoclickerP2Cps > 0.f) {
            float interval = 1.0f / c.autoclickerP2Cps / 2.0f;
            m_fields->timeP2 += dt;
            while (m_fields->timeP2 >= interval) {
                m_fields->stateP2 = !m_fields->stateP2;
                m_fields->timeP2 -= interval;
                this->handleButton(m_fields->stateP2, (int)PlayerButton::Jump, false);
            }
        }

        GJBaseGameLayer::processQueuedButtons(dt, clearInputQueue);
    }

    void resetLevelVariables() {
        GJBaseGameLayer::resetLevelVariables();
        m_fields->timeP1  = 0.0;
        m_fields->timeP2  = 0.0;
        m_fields->stateP1 = false;
        m_fields->stateP2 = false;
    }
};
