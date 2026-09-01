#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include "../Config.hpp"

using namespace geode::prelude;

class $modify(NHShakeLayer, GJBaseGameLayer) {
    void shakeCamera(float duration, float strength, float interval) {
        auto& cfg = Config::get();
        if (cfg.noCameraShake) return;
        if (cfg.noDeathShake && (m_player1 && m_player1->m_isDead)) return;
        if (cfg.noEndShake && m_levelEndAnimationStarted) return;
        GJBaseGameLayer::shakeCamera(duration, strength, interval);
    }
};
