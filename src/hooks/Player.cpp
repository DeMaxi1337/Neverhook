#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/PlayerObject.hpp>
#include "../Config.hpp"

using namespace geode::prelude;

class $modify(NHPlayLayer, PlayLayer) {
    void destroyPlayer(PlayerObject* player, GameObject* object) {
        if (Config::get().noclip) return;
        PlayLayer::destroyPlayer(player, object);
    }
};

class $modify(NHPlayerObject, PlayerObject) {
    void playDeathEffect() {
        if (Config::get().noDeathEffect) return;
        PlayerObject::playDeathEffect();
    }
    void playSpawnEffect() {
        if (Config::get().noRespawnFlash) return;
        PlayerObject::playSpawnEffect();
    }
};
