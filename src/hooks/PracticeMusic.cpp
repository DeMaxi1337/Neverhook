#include <Geode/Geode.hpp>
#include <Geode/modify/GameStatsManager.hpp>
#include "../Config.hpp"

using namespace geode::prelude;

// Practice Music -- unlock the practice-mode music toggle.
class $modify(NHGameStatsManager, GameStatsManager) {
    bool isItemUnlocked(UnlockType type, int id) {
        if (Config::get().practiceMusic && (int)type == 12 && id == 17)
            return true;
        return GameStatsManager::isItemUnlocked(type, id);
    }
};
