#include <Geode/Geode.hpp>
#include <Geode/modify/LevelInfoLayer.hpp>

#include "../Config.hpp"

using namespace geode::prelude;

class $modify(NHCopyHack, LevelInfoLayer) {
    bool init(GJGameLevel* level, bool challenge) {
        auto saved = level->m_password.value();
        if (Config::get().copyHack)
            level->m_password = 1;

        bool ok = LevelInfoLayer::init(level, challenge);

        level->m_password = saved;
        return ok;
    }

    void levelDownloadFinished(GJGameLevel* level) {
        auto saved = level->m_password.value();
        if (Config::get().copyHack)
            level->m_password = 1;

        LevelInfoLayer::levelDownloadFinished(level);

        level->m_password = saved;
    }

    void tryCloneLevel(cocos2d::CCObject* sender) {
        auto saved = m_level->m_password.value();
        if (Config::get().copyHack)
            m_level->m_password = 1;

        LevelInfoLayer::tryCloneLevel(sender);

        m_level->m_password = saved;
    }
};
