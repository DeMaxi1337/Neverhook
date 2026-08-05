#include <Geode/Geode.hpp>
#include <Geode/modify/PlayerObject.hpp>
#include "../Config.hpp"

using namespace geode::prelude;

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
