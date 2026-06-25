#include <Geode/Geode.hpp>
#include <Geode/modify/GameManager.hpp>
#include <Geode/modify/GameStatsManager.hpp>
#include "../Config.hpp"

using namespace geode::prelude;

class $modify(NHGameManager, GameManager) {
    bool isIconUnlocked(int id, IconType type) {
        if (Config::get().iconBypass) return true;
        return GameManager::isIconUnlocked(id, type);
    }
    bool isColorUnlocked(int id, UnlockType type) {
        if (Config::get().iconBypass) return true;
        return GameManager::isColorUnlocked(id, type);
    }
};

class $modify(NHGameStatsManager, GameStatsManager) {
    bool isItemUnlocked(UnlockType type, int id) {
        if (Config::get().practiceMusic && (int)type == 12 && id == 17)
            return true;
        return GameStatsManager::isItemUnlocked(type, id);
    }
};
