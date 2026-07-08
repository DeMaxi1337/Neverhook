#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/binding/EndTriggerGameObject.hpp>
#include "../Config.hpp"

using namespace geode::prelude;

// Keeps the pause button usable during the level-complete animation, so a run
// can be paused/restarted even after the end trigger fires.
class $modify(NHPauseDuringComplete, PlayLayer) {
    void pauseGame(bool p0) {
        if (!Config::get().pauseDuringComplete)
            return PlayLayer::pauseGame(p0);

        bool prev = m_levelEndAnimationStarted;
        m_levelEndAnimationStarted = false;
        PlayLayer::pauseGame(p0);
        m_levelEndAnimationStarted = prev;
    }

    void resetLevel() {
        if (Config::get().pauseDuringComplete && m_levelEndAnimationStarted) {
            if (m_player1) m_player1->stopAllActions();
            if (m_player2) m_player2->stopAllActions();
        }
        PlayLayer::resetLevel();
    }

    void activatePlatformerEndTrigger(EndTriggerGameObject* p0, gd::vector<int> const& p1) {
        PlayLayer::activatePlatformerEndTrigger(p0, p1);
        if (Config::get().pauseDuringComplete && m_uiLayer && m_uiLayer->m_pauseBtn)
            m_uiLayer->m_pauseBtn->setEnabled(true);
    }

    void checkForEnd() {
        PlayLayer::checkForEnd();
        if (Config::get().pauseDuringComplete && m_uiLayer && m_uiLayer->m_pauseBtn)
            m_uiLayer->m_pauseBtn->setEnabled(true);
    }
};
