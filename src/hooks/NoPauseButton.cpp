#include <Geode/Geode.hpp>
#include <Geode/modify/UILayer.hpp>
#include "../Config.hpp"

using namespace geode::prelude;

class $modify(NHNoPauseButton, UILayer) {
    bool init(GJBaseGameLayer* layer) {
        if (!UILayer::init(layer)) return false;
        this->schedule(schedule_selector(NHNoPauseButton::updatePauseBtn));
        return true;
    }

    void updatePauseBtn(float dt) {
        if (m_pauseBtn) {
            bool target = !Config::get().noPauseButton;
            if (m_pauseBtn->isVisible() != target) {
                m_pauseBtn->setVisible(target);
            }
        }
    }
};
