#include <Geode/Geode.hpp>
#include <Geode/modify/CCScheduler.hpp>
#include "../Config.hpp"

using namespace geode::prelude;

class $modify(cocos2d::CCScheduler) {
    void update(float dt) {
        auto& c = Config::get();
        if (c.speedhack) dt *= c.speedhackValue;
        CCScheduler::update(dt);
    }
};
