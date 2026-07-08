#include <Geode/Geode.hpp>
#include <Geode/modify/GameManager.hpp>
#include <Geode/modify/GameStatsManager.hpp>
#include <Geode/modify/CreatorLayer.hpp>
#include <Geode/modify/OptionsLayer.hpp>
#include <Geode/modify/SecretLayer2.hpp>
#include <Geode/modify/LevelPage.hpp>
#include "../Config.hpp"

#include <algorithm>

using namespace geode::prelude;

// Opens the secret vaults (Vault, Treasure Room, Vault of Secrets, Chamber of
// Time) by spoofing the specific unlock/stat checks the doors perform. The
// static flags arm a single bypassed check right before each door opens so
// unrelated lookups are unaffected.

namespace {
    bool s_bypassUGV = false;
    bool s_bypassUGV2 = false;
    bool s_bypassUGVSkip = false;
    bool s_bypassGameStat = false;
    int  s_bypassGameStatValue = 0;

    bool vaultsOn() { return Config::get().unlockVaults; }
}

class $modify(NHUnlockVaultsGM, GameManager) {
    bool getUGV(const char* key) {
        bool result = GameManager::getUGV(key);
        if (!vaultsOn()) return result;

        if (s_bypassUGVSkip) {
            s_bypassUGVSkip = false;
            s_bypassUGV = true;
            return result;
        } else if (s_bypassUGV2) {
            s_bypassUGV2 = false;
            s_bypassUGV = true;
            return true;
        } else if (s_bypassUGV) {
            s_bypassUGV = false;
            return true;
        }
        return result;
    }
};

class $modify(NHUnlockVaultsGSM, GameStatsManager) {
    int getStat(const char* key) {
        int value = GameStatsManager::getStat(key);
        if (!vaultsOn() || !s_bypassGameStat) return value;
        s_bypassGameStat = false;
        return std::max(value, s_bypassGameStatValue);
    }

    bool isItemUnlocked(UnlockType type, int key) {
        if (GameStatsManager::isItemUnlocked(type, key)) return true;
        if (!vaultsOn()) return false;
        return type == UnlockType::GJItem && key >= 1 && key <= 4;
    }
};

class $modify(NHUnlockVaultsCL, CreatorLayer) {
    void onSecretVault(cocos2d::CCObject* sender) {
        if (vaultsOn()) {
            s_bypassGameStat = true;
            s_bypassGameStatValue = 51; // key 13 > 50
        }
        CreatorLayer::onSecretVault(sender);
    }

    void onTreasureRoom(cocos2d::CCObject* sender) {
        if (vaultsOn()) s_bypassUGV = true;
        CreatorLayer::onTreasureRoom(sender);
    }

    bool init() {
        if (vaultsOn()) {
            s_bypassUGV = true;
            s_bypassGameStat = true;
            s_bypassGameStatValue = 51; // key 13 > 50
        }
        return CreatorLayer::init();
    }
};

class $modify(NHUnlockVaultsOL, OptionsLayer) {
    void onSecretVault(cocos2d::CCObject* sender) {
        if (vaultsOn()) {
            s_bypassGameStat = true;
            s_bypassGameStatValue = 11; // key 12 > 10
        }
        OptionsLayer::onSecretVault(sender);
    }
};

class $modify(NHUnlockVaultsSL2, SecretLayer2) {
    void onDoor(cocos2d::CCObject* sender) {
        if (vaultsOn()) s_bypassUGV = true;
        SecretLayer2::onDoor(sender);
    }
};

class $modify(NHUnlockVaultsLP, LevelPage) {
    void addSecretDoor() {
        if (vaultsOn()) s_bypassUGV = true;
        LevelPage::addSecretDoor();
    }
};
