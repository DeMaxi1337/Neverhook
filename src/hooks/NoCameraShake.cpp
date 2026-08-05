#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include "../Config.hpp"

using namespace geode::prelude;

class $modify(NHShakeLayer, GJBaseGameLayer) {
    void shakeCamera(float duration, float strength, float interval) {
        if (Config::get().noCameraShake) return;
        GJBaseGameLayer::shakeCamera(duration, strength, interval);
    }
};
