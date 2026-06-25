#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include "../Config.hpp"

#include <algorithm>
#include <cmath>

using namespace geode::prelude;

namespace {
    double targetTps() {
        return std::clamp(static_cast<double>(Config::get().tpsValue), 1.0, 100000.0);
    }

    int passesThisFrame(double tps) {
        double frame = CCDirector::get()->getDeltaTime();
        if (frame <= 0.0) frame = 1.0 / 60.0;
        int passes = static_cast<int>(std::ceil(tps * frame));
        passes = std::clamp(passes, 1, 2000); // guard against absurd input
        return passes;
    }
}

class $modify(NHTpsLayer, GJBaseGameLayer) {
    double getModifiedDelta(float dt) {
        auto& c = Config::get();
        if (!c.tpsBypass || m_isEditor)
            return GJBaseGameLayer::getModifiedDelta(dt);

        double tps    = targetTps();
        double stride = (1.0 / tps) * std::min<float>(m_gameState.m_timeWarp, 1.0f);
        if (stride <= 0.0)
            return GJBaseGameLayer::getModifiedDelta(dt);

        int passes = passesThisFrame(tps);

        double pool;
        if (m_resumeTimer < 1) {
            pool = m_extraDelta + static_cast<double>(dt) / passes;
        } else {
            --m_resumeTimer;
            pool = m_extraDelta;
        }

        double consumed = stride * static_cast<int>(pool / stride);
        m_extraDelta = pool - consumed;
        return consumed;
    }

    void update(float dt) {
        auto& c = Config::get();
        if (c.tpsBypass && m_started && !m_isEditor) {
            int passes = passesThisFrame(targetTps());
            for (int i = 0; i < passes; ++i)
                GJBaseGameLayer::update(dt);
            return;
        }
        GJBaseGameLayer::update(dt);
    }
};
