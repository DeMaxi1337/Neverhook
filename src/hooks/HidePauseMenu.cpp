#include <Geode/Geode.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include "../Config.hpp"

using namespace geode::prelude;

class $modify(NHHidePauseMenu, PauseLayer) {
    void customSetup() {
        PauseLayer::customSetup();
        this->schedule(schedule_selector(NHHidePauseMenu::updateHidePauseMenu));
        if (Config::get().hidePauseMenu) {
            this->setVisible(false);
        }
    }

    void updateHidePauseMenu(float dt) {
        bool targetVis = !Config::get().hidePauseMenu;
        if (this->isVisible() != targetVis) {
            this->setVisible(targetVis);
        }
    }
};
