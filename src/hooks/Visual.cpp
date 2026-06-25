#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/PlayerObject.hpp>
#include "../Config.hpp"

#include <cstdio>

using namespace geode::prelude;

class $modify(NHShakeLayer, GJBaseGameLayer) {
    void shakeCamera(float duration, float strength, float interval) {
        if (Config::get().noCameraShake) return;
        GJBaseGameLayer::shakeCamera(duration, strength, interval);
    }
};

class $modify(NHGlowPlayer, PlayerObject) {
    void updatePlayerGlow() {
        PlayerObject::updatePlayerGlow();
        if (Config::get().noGlow) {
            m_hasGlow = false;
            if (m_iconGlow)    m_iconGlow->setVisible(false);
            if (m_vehicleGlow) m_vehicleGlow->setVisible(false);
        }
    }
};

class $modify(NHVisualPlayLayer, PlayLayer) {
    void postUpdate(float dt) {
        auto& c = Config::get();
        PlayLayer::postUpdate(dt);

        if (m_attemptLabel)
            m_attemptLabel->setVisible(!c.hideAttempts);

        if (c.accuratePercent && m_percentageLabel) {
            int digits = c.accuratePercentDigits;
            if (digits < 0) digits = 0;
            if (digits > 6) digits = 6;
            char buf[32];
            std::snprintf(buf, sizeof(buf), "%.*f%%", digits, getCurrentPercent());
            m_percentageLabel->setString(buf);
        }
    }
};
