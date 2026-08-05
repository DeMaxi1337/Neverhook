#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include "../Config.hpp"

using namespace geode::prelude;

class $modify(NHNoMirror, GJBaseGameLayer) {
    void toggleFlipped(bool flip, bool noEffects) {
        if (Config::get().noMirrorPortal)
            return;
        GJBaseGameLayer::toggleFlipped(flip, noEffects);
    }
};
