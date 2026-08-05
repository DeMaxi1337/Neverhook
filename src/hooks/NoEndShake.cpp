#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include "../Config.hpp"

using namespace geode::prelude;

class $modify(NHNoEndShake, PlayLayer) {
    void showCompleteEffect() {
        auto& cfg = Config::get();
        if (!cfg.noEndShake && !cfg.noCameraShake) {
            PlayLayer::showCompleteEffect();
            return;
        }

        const bool  prevEnabled = m_gameState.m_cameraShakeEnabled;
        const float prevFactor  = m_gameState.m_cameraShakeFactor;

        PlayLayer::showCompleteEffect();

        m_gameState.m_cameraShakeEnabled = prevEnabled;
        m_gameState.m_cameraShakeFactor  = prevFactor;
        this->stopCameraShake();
    }
};
