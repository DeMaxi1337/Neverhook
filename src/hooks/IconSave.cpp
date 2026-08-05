#include <Geode/Geode.hpp>
#include <Geode/modify/GameManager.hpp>
#include <Geode/modify/GJGarageLayer.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include "../Config.hpp"

using namespace geode::prelude;

static void nhSaveCosmetics() {
    auto gm = GameManager::get();
    if (!gm) return;
    auto m = Mod::get();
    m->setSavedValue<int>("ic_frame",   gm->getPlayerFrame());
    m->setSavedValue<int>("ic_ship",    gm->getPlayerShip());
    m->setSavedValue<int>("ic_ball",    gm->getPlayerBall());
    m->setSavedValue<int>("ic_bird",    gm->getPlayerBird());
    m->setSavedValue<int>("ic_dart",    gm->getPlayerDart());
    m->setSavedValue<int>("ic_robot",   gm->getPlayerRobot());
    m->setSavedValue<int>("ic_spider",  gm->getPlayerSpider());
    m->setSavedValue<int>("ic_swing",   gm->getPlayerSwing());
    m->setSavedValue<int>("ic_jetpack", gm->getPlayerJetpack());
    m->setSavedValue<int>("ic_color",   gm->getPlayerColor());
    m->setSavedValue<int>("ic_color2",  gm->getPlayerColor2());
    m->setSavedValue<int>("ic_glowcol", gm->getPlayerGlowColor());
    m->setSavedValue<int>("ic_streak",  gm->getPlayerStreak());
    m->setSavedValue<int>("ic_death",   gm->getPlayerDeathEffect());
    m->setSavedValue<bool>("ic_glow",   gm->getPlayerGlow());
    m->setSavedValue<bool>("ic_has", true);
}

static void nhRestoreCosmetics() {
    auto gm = GameManager::get();
    if (!gm) return;
    auto m = Mod::get();
    if (!m->getSavedValue<bool>("ic_has", false)) return;

    gm->setPlayerFrame(      m->getSavedValue<int>("ic_frame",   gm->getPlayerFrame()));
    gm->setPlayerShip(       m->getSavedValue<int>("ic_ship",    gm->getPlayerShip()));
    gm->setPlayerBall(       m->getSavedValue<int>("ic_ball",    gm->getPlayerBall()));
    gm->setPlayerBird(       m->getSavedValue<int>("ic_bird",    gm->getPlayerBird()));
    gm->setPlayerDart(       m->getSavedValue<int>("ic_dart",    gm->getPlayerDart()));
    gm->setPlayerRobot(      m->getSavedValue<int>("ic_robot",   gm->getPlayerRobot()));
    gm->setPlayerSpider(     m->getSavedValue<int>("ic_spider",  gm->getPlayerSpider()));
    gm->setPlayerSwing(      m->getSavedValue<int>("ic_swing",   gm->getPlayerSwing()));
    gm->setPlayerJetpack(    m->getSavedValue<int>("ic_jetpack", gm->getPlayerJetpack()));
    gm->setPlayerColor(      m->getSavedValue<int>("ic_color",   gm->getPlayerColor()));
    gm->setPlayerColor2(     m->getSavedValue<int>("ic_color2",  gm->getPlayerColor2()));
    gm->setPlayerColor3(     m->getSavedValue<int>("ic_glowcol", gm->getPlayerGlowColor()));
    gm->setPlayerStreak(     m->getSavedValue<int>("ic_streak",  gm->getPlayerStreak()));
    gm->setPlayerDeathEffect(m->getSavedValue<int>("ic_death",   gm->getPlayerDeathEffect()));
    gm->setPlayerGlow(       m->getSavedValue<bool>("ic_glow",   gm->getPlayerGlow()));
}

class $modify(NHIconSaveGM, GameManager) {
    bool init() {
        if (!GameManager::init()) return false;
        if (Mod::get()->getSavedValue<bool>("iconBypass", false))
            nhRestoreCosmetics();
        return true;
    }
};

class $modify(NHIconSaveMenu, MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) return false;
        if (Mod::get()->getSavedValue<bool>("iconBypass", false))
            nhRestoreCosmetics();
        return true;
    }
};

class $modify(NHIconSaveGarage, GJGarageLayer) {
    void onBack(cocos2d::CCObject* sender) {
        if (Config::get().iconBypass)
            nhSaveCosmetics();
        GJGarageLayer::onBack(sender);
    }
};
