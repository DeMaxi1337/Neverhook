#include <Geode/Geode.hpp>
#include <Geode/modify/GameManager.hpp>
#include <Geode/modify/GameStatsManager.hpp>
#include "../Config.hpp"

using namespace geode::prelude;

class $modify(NHGameManager, GameManager) {
    bool isIconUnlocked(int key, IconType type) {
        if (GameManager::isIconUnlocked(key, type)) return true;
        return Config::get().iconBypass;
    }
    bool isColorUnlocked(int key, UnlockType type) {
        if (GameManager::isColorUnlocked(key, type)) return true;
        return Config::get().iconBypass;
    }
};

class $modify(NHGlowUnlock, GameStatsManager) {
    bool isItemUnlocked(UnlockType type, int key) {
        if (GameStatsManager::isItemUnlocked(type, key)) return true;
        if (Config::get().iconBypass)
            return type == UnlockType::GJItem && (key >= 18 && key <= 20);
        return false;
    }
};
