#include <Geode/Geode.hpp>
#include <Geode/modify/GameManager.hpp>
#include "../Config.hpp"

using namespace geode::prelude;

// Icon Bypass -- unlock every icon and color in the icon kit.
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
