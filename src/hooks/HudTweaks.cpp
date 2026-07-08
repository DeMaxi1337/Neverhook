#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <cstdio>
#include "../Config.hpp"

using namespace geode::prelude;

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
