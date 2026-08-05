#include <Geode/Geode.hpp>
#include <Geode/modify/GameLevelManager.hpp>
#include "../Config.hpp"

using namespace geode::prelude;

class $modify(NHUnlockMainLevels, GameLevelManager) {
    GJGameLevel* getMainLevel(int levelID, bool dontGetLevelString) {
        auto* level = GameLevelManager::getMainLevel(levelID, dontGetLevelString);
        if (Config::get().unlockMainLevels && level && level->m_requiredCoins > 0)
            level->m_requiredCoins = 0;
        return level;
    }
};
