#include <Geode/Geode.hpp>
#include <Geode/modify/CCCircleWave.hpp>
#include "../Config.hpp"

using namespace geode::prelude;

// Hides CCCircleWave ring effects (orbs, pads, portals, gravity spheres, etc.)
// during gameplay for a cleaner look. Menus are left untouched by requiring an
// active PlayLayer.
class $modify(NHNoCircleWave, CCCircleWave) {
    void setPosition(cocos2d::CCPoint const& p0) {
        CCCircleWave::setPosition(p0);
        if (!Config::get().noCircleWave) return;
        if (PlayLayer::get())
            this->setVisible(false);
    }
};
