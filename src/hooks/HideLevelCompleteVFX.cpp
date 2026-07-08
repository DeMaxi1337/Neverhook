#include <Geode/Geode.hpp>
#include <Geode/modify/CCCircleWave.hpp>
#include <Geode/modify/CCLightFlash.hpp>
#include <Geode/modify/CCParticleSystem.hpp>
#include <Geode/binding/CurrencyRewardLayer.hpp>
#include "../Config.hpp"

using namespace geode::prelude;

// Hides the burst of circles, light flashes and particles that play when a
// level is completed, while leaving the same effects intact during gameplay
// (guarded by PlayLayer::m_levelEndAnimationStarted).

namespace {
    bool hideVfxActive() {
        return Config::get().hideLevelCompleteVfx;
    }
}

class $modify(NHHideCompleteCircle, CCCircleWave) {
    void setPosition(cocos2d::CCPoint const& p0) {
        CCCircleWave::setPosition(p0);
        if (!hideVfxActive()) return;
        auto* pl = PlayLayer::get();
        if (!pl) return;
        if (pl->m_levelEndAnimationStarted && !typeinfo_cast<CurrencyRewardLayer*>(this->getParent()))
            this->setVisible(false);
    }
};

class $modify(NHHideCompleteFlash, CCLightFlash) {
    void playEffect(cocos2d::CCPoint point, cocos2d::ccColor3B color, float p2, float p3, float p4,
                    float p5, float p6, float p7, float p8, float p9, float p10, float p11, float p12,
                    float p13, float p14, float p15, int p16, bool p17, bool p18, float p19) {
        CCLightFlash::playEffect(point, color, p2, p3, p4, p5, p6, p7, p8, p9, p10,
                                 p11, p12, p13, p14, p15, p16, p17, p18, p19);
        if (!hideVfxActive()) return;
        auto* pl = PlayLayer::get();
        if (!pl) return;
        if (pl->m_levelEndAnimationStarted)
            this->setVisible(false);
    }
};

class $modify(NHHideCompleteParticle, cocos2d::CCParticleSystem) {
    void initParticle(cocos2d::sCCParticle* p0) {
        CCParticleSystem::initParticle(p0);
        if (!hideVfxActive()) return;
        auto* pl = PlayLayer::get();
        if (!pl) return;
        if (this->getParent() != pl) return;
        if (!typeinfo_cast<cocos2d::CCParticleSystemQuad*>(this)) return;
        if (pl->m_levelEndAnimationStarted)
            this->setVisible(false);
    }
};
