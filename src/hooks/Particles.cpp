#include <Geode/Geode.hpp>
#include <Geode/modify/CCParticleSystemQuad.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include "../Config.hpp"

using namespace geode::prelude;

class $modify(NHParticleSystem, CCParticleSystemQuad) {
    void draw() {
        if (Config::get().noParticles
                && PlayLayer::get()
                && !typeinfo_cast<CCParticleSnow*>(this)) {
            return;
        }
        CCParticleSystemQuad::draw();
    }

    static void onModify(auto& self) {
        (void)self.setHookPriorityPost("cocos2d::CCParticleSystemQuad::draw", Priority::Last);
    }
};
