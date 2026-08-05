#include <Geode/Geode.hpp>
#include <Geode/modify/GameManager.hpp>
#include "../Config.hpp"

#include <cstring>

using namespace geode::prelude;

class $modify(NHUnlockShops, GameManager) {
    bool getUGV(const char* key) {
        if (GameManager::getUGV(key)) return true;
        if (!Config::get().unlockShops) return false;
        return std::strcmp(key, "11") == 0 || std::strcmp(key, "20") == 0
            || std::strcmp(key, "34") == 0 || std::strcmp(key, "35") == 0;
    }
};
