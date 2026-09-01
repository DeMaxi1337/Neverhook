#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include "../Config.hpp"

using namespace geode::prelude;

class $modify(NHNoDeathShake, PlayLayer) {
    void destroyPlayer(PlayerObject* player, GameObject* object) {
        auto& cfg = Config::get();
        if (!cfg.noDeathShake && !cfg.noCameraShake) {
            PlayLayer::destroyPlayer(player, object);
            return;
        }

        const bool  prevEnabled = m_gameState.m_cameraShakeEnabled;
        const float prevFactor  = m_gameState.m_cameraShakeFactor;

        PlayLayer::destroyPlayer(player, object);

        m_gameState.m_cameraShakeEnabled = prevEnabled;
        m_gameState.m_cameraShakeFactor  = prevFactor;
        this->stopCameraShake();
    }
};
