#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include "../Config.hpp"

using namespace geode::prelude;

// No Camera Shake: block gameplay camera shakes at the source. shakeCamera is
// where the game schedules a shake, so skipping it prevents the shake from ever
// starting. The level-complete shake goes through a different path and is
// handled in NoEndShake.cpp (which also honours this toggle).
class $modify(NHShakeLayer, GJBaseGameLayer) {
    void shakeCamera(float duration, float strength, float interval) {
        if (Config::get().noCameraShake) return;
        GJBaseGameLayer::shakeCamera(duration, strength, interval);
    }
};
