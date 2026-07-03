#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include "../Config.hpp"

using namespace geode::prelude;

// Autoclicker (P1 / P2).
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
