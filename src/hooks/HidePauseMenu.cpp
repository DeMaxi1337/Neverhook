#include <Geode/Geode.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include "../Config.hpp"

using namespace geode::prelude;

class $modify(NHHidePauseMenu, PauseLayer) {
    void customSetup() {
        PauseLayer::customSetup();
        if (Config::get().hidePauseMenu) {
            this->setVisible(false);
        }
    }
};
