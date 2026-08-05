#include <Geode/Geode.hpp>
#include <Geode/modify/LevelInfoLayer.hpp>
#include "../Config.hpp"

using namespace geode::prelude;

class $modify(NHAutoSongDownload, LevelInfoLayer) {
    void tryDownload() {
        if (!Config::get().autoSongDownload) return;
        if (m_songWidget && m_songWidget->m_downloadBtn && m_songWidget->m_downloadBtn->isVisible())
            m_songWidget->m_downloadBtn->activate();
    }

    void levelDownloadFinished(GJGameLevel* level) {
        LevelInfoLayer::levelDownloadFinished(level);
        tryDownload();
    }

    void onEnterTransitionDidFinish() {
        LevelInfoLayer::onEnterTransitionDidFinish();
        if (m_level && m_level->m_dailyID > 0)
            tryDownload();
    }
};
