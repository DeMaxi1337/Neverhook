#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include "../Config.hpp"

using namespace geode::prelude;

// No End Shake: suppress the camera shake played on level completion.
//
// The completion shake is not scheduled through shakeCamera(); showCompleteEffect
// raises GJGameState::m_cameraShakeFactor, which the camera then applies over the
// following frames. To cancel it we snapshot the shake state, run the original
// effect, then restore the pre-effect state and stop any shake that was queued.
//
// The global No Camera Shake toggle reuses this path so it also covers the
// completion shake, while normal gameplay shakes are handled in NoCameraShake.cpp.
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
