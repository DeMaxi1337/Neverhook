#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/PlayerObject.hpp>
#include <Geode/binding/GameStatsManager.hpp>

#include "../Config.hpp"

using namespace geode::prelude;

class $modify(NHSafeModePL, PlayLayer) {
    struct Fields {
        int keepJumps = 0;
        int keepAttempts = 0;
    };

    bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
        if (auto gsm = GameStatsManager::get()) {
            m_fields->keepJumps = gsm->getStat("1");
            m_fields->keepAttempts = gsm->getStat("2");
        }
        return PlayLayer::init(level, useReplay, dontCreateObjects);
    }

    void destroyPlayer(PlayerObject* player, GameObject* object) {
        auto& c = Config::get();
        bool prevTest = m_isTestMode;

        if (c.safeMode) {
            m_isTestMode = true;
            if (auto gsm = GameStatsManager::get()) {
                if (c.safeFreezeJumps)    gsm->setStat("1", m_fields->keepJumps);
                if (c.safeFreezeAttempts) gsm->setStat("2", m_fields->keepAttempts);
            }
        }

        PlayLayer::destroyPlayer(player, object);
        m_isTestMode = prevTest;
    }

    void levelComplete() {
        auto& c = Config::get();
        bool prevTest = m_isTestMode;

        if (c.safeMode)
            m_isTestMode = true;

        PlayLayer::levelComplete();
        m_isTestMode = prevTest;
    }

    void resetLevel() {
        auto& c = Config::get();

        if (c.safeMode && c.safeFreezeAttempts && m_level)
            m_level->m_attempts = m_level->m_attempts.value() - 1;

        PlayLayer::resetLevel();
    }
};

class $modify(NHSafeModePO, PlayerObject) {
    void incrementJumps() {
        auto& c = Config::get();
        if (c.safeMode && c.safeFreezeJumps)
            return;
        PlayerObject::incrementJumps();
    }
};
