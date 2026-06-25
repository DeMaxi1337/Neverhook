#include <Geode/Geode.hpp>
#include <Geode/modify/ShareLevelLayer.hpp>
#include "../Config.hpp"

using namespace geode::prelude;

class $modify(NHShareLevelLayer, ShareLevelLayer) {
    struct Fields {
        GJGameLevel* level = nullptr;
    };

    bool init(GJGameLevel* level) {
        if (!ShareLevelLayer::init(level)) return false;
        m_fields->level = level;
        return true;
    }

    void onShare(cocos2d::CCObject* sender) {
        auto level = m_fields->level;
        if (!Config::get().verifyHack || !level) {
            ShareLevelLayer::onShare(sender);
            return;
        }
        auto p1 = level->m_isVerifiedRaw;
        auto p2 = level->m_isVerified.value();
        level->m_isVerifiedRaw = true;
        level->m_isVerified = true;
        ShareLevelLayer::onShare(sender);
        level->m_isVerifiedRaw = p1;
        level->m_isVerified = p2;
    }
};
