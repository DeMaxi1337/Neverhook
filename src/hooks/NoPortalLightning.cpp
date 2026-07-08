#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/binding/GameManager.hpp>
#include "../Config.hpp"

using namespace geode::prelude;

// Suppresses the full-screen lightning flash from mirror/other portals by
// forcing performance mode for the duration of the flash call.
class $modify(NHNoPortalLightning, GJBaseGameLayer) {
    void lightningFlash(cocos2d::CCPoint from, cocos2d::CCPoint to, cocos2d::ccColor3B color,
                        float lineWidth, float duration, int displacement, bool flash, float opacity) {
        if (!Config::get().noPortalLightning)
            return GJBaseGameLayer::lightningFlash(from, to, color, lineWidth, duration, displacement, flash, opacity);

        auto* gm = GameManager::get();
        bool prev = gm->m_performanceMode;
        gm->m_performanceMode = true;
        GJBaseGameLayer::lightningFlash(from, to, color, lineWidth, duration, displacement, flash, opacity);
        gm->m_performanceMode = prev;
    }
};
