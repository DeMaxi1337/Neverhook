#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include "../Config.hpp"

using namespace geode::prelude;

// Enters practice mode automatically as soon as a level starts.
class $modify(NHAutoPracticeMode, PlayLayer) {
    bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
        if (!PlayLayer::init(level, useReplay, dontCreateObjects))
            return false;
        if (Config::get().autoPracticeMode)
            this->togglePracticeMode(true);
        return true;
    }
};
