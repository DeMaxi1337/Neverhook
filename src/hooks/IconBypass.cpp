#include <Geode/Geode.hpp>
#include <Geode/modify/GameManager.hpp>
#include <Geode/modify/GameStatsManager.hpp>
#include "../Config.hpp"

using namespace geode::prelude;

// Icon Bypass only removes the ownership gate so any icon, color or glow item
// can be selected and shown. It does NOT force glow rendering: the player's
// glow is drawn natively by the game according to the garage Glow checkbox.
// (Glow was previously being hidden by the No Glow feature, not by unlocking.)
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
