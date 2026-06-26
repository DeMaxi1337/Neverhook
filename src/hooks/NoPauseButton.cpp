#include <Geode/Geode.hpp>
#include <Geode/modify/UILayer.hpp>
#include "../Config.hpp"

using namespace geode::prelude;

class $modify(NHNoPauseButton, UILayer) {
    bool init(GJBaseGameLayer* layer) {
        if (!UILayer::init(layer)) return false;
        if (Config::get().noPauseButton && m_pauseBtn)
            m_pauseBtn->setVisible(false);
        return true;
    }
};
