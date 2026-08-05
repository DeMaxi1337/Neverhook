
#include "LuaEngine.hpp"
#include "LuaHooks.hpp"

#include <Geode/Geode.hpp>
#include <Geode/binding/CheckpointObject.hpp>
#include <Geode/binding/EffectGameObject.hpp>
#include <Geode/binding/GJGameLevel.hpp>
#include <Geode/binding/GameObject.hpp>
#include <Geode/binding/HardStreak.hpp>
#include <Geode/binding/PlayLayer.hpp>
#include <Geode/binding/PlayerObject.hpp>
#include <Geode/binding/RingObject.hpp>
#include <Geode/modify/AppDelegate.hpp>
#include <Geode/modify/CreatorLayer.hpp>
#include <Geode/modify/EffectGameObject.hpp>
#include <Geode/modify/FMODAudioEngine.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/GameManager.hpp>
#include <Geode/modify/GameObject.hpp>
#include <Geode/modify/HardStreak.hpp>
#include <Geode/modify/LevelEditorLayer.hpp>
#include <Geode/modify/LevelInfoLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/PlayerObject.hpp>
#include <Geode/modify/UILayer.hpp>
#include <Geode/modify/CCKeyboardDispatcher.hpp>

using namespace geode::prelude;

using nh::lua::dispatchHook;
using nh::lua::HookEvent;
using nh::lua::HookId;
using nh::lua::hookHasListeners;

namespace {

bool listening(HookId id) { return hookHasListeners(id); }

bool fire(HookId id) { return dispatchHook(id); }

bool fire(HookId id, const HookEvent& event) { return dispatchHook(id, event); }

int playerIndex(PlayerObject* player) {
    auto* game = GJBaseGameLayer::get();
    if (!game || !player) return 0;
    if (player == game->m_player1) return 1;
    if (player == game->m_player2) return 2;
    return 0;
}

void fillPlayer(HookEvent& event, PlayerObject* player) {
    event.setNumber("player", (double)playerIndex(player));
    if (!player) return;

    event.setNumber("x", player->getPositionX());
    event.setNumber("y", player->getPositionY());
    event.setNumber("rotation", player->getRotation());
    event.setNumber("y_velocity", (double)player->m_yVelocity);
    event.setBool("on_ground", player->m_isOnGround);
    event.setBool("upside_down", player->m_isUpsideDown);
    event.setBool("dead", player->m_isDead);
}

void fillObject(HookEvent& event, GameObject* object) {
    if (!object) {
        event.setNumber("object_id", 0.0);
        return;
    }

    event.setNumber("object_id", (double)object->m_objectID);
    event.setNumber("object_x", object->getPositionX());
    event.setNumber("object_y", object->getPositionY());
}

}

class $modify(NHLuaAllPlayLayer, PlayLayer) {
    void postUpdate(float dt) {
        PlayLayer::postUpdate(dt);

        if (!listening(HookId::PlayLayerPostUpdate)) return;

        HookEvent event;
        event.setNumber("dt", (double)dt);
        event.setNumber("percent", this->getCurrentPercent());
        fire(HookId::PlayLayerPostUpdate, event);
    }

    void resetLevelFromStart() {
        PlayLayer::resetLevelFromStart();
        if (listening(HookId::PlayLayerResetFromStart))
            fire(HookId::PlayLayerResetFromStart);
    }

    void fullReset() {
        PlayLayer::fullReset();
        if (listening(HookId::PlayLayerFullReset))
            fire(HookId::PlayLayerFullReset);
    }

    void pauseGame(bool p0) {
        if (listening(HookId::PlayLayerPauseGame)) {
            HookEvent event;
            event.setBool("menu", p0);
            if (!fire(HookId::PlayLayerPauseGame, event)) return;
        }

        PlayLayer::pauseGame(p0);
    }

    void onQuit() {
        if (listening(HookId::PlayLayerOnQuit))
            fire(HookId::PlayLayerOnQuit);

        PlayLayer::onQuit();
    }

    void showCompleteEffect() {
        if (listening(HookId::PlayLayerCompleteEffect) &&
            !fire(HookId::PlayLayerCompleteEffect))
            return;

        PlayLayer::showCompleteEffect();
    }

    void storeCheckpoint(CheckpointObject* cp) {
        if (listening(HookId::PlayLayerStoreCheckpoint)) {
            HookEvent event;
            event.setNumber("percent", this->getCurrentPercent());
            if (!fire(HookId::PlayLayerStoreCheckpoint, event)) return;
        }

        PlayLayer::storeCheckpoint(cp);
    }

    void loadFromCheckpoint(CheckpointObject* cp) {
        if (listening(HookId::PlayLayerLoadCheckpoint)) {
            HookEvent event;
            event.setNumber("percent", this->getCurrentPercent());
            if (!fire(HookId::PlayLayerLoadCheckpoint, event)) return;
        }

        PlayLayer::loadFromCheckpoint(cp);
    }

    void removeCheckpoint(bool first) {
        if (listening(HookId::PlayLayerRemoveCheckpoint)) {
            HookEvent event;
            event.setBool("first", first);
            if (!fire(HookId::PlayLayerRemoveCheckpoint, event)) return;
        }

        PlayLayer::removeCheckpoint(first);
    }

    void playEndAnimationToPos(cocos2d::CCPoint pos) {
        if (listening(HookId::PlayLayerEndAnimation)) {
            HookEvent event;
            event.setNumber("x", pos.x);
            event.setNumber("y", pos.y);
            if (!fire(HookId::PlayLayerEndAnimation, event)) return;
        }

        PlayLayer::playEndAnimationToPos(pos);
    }

    void addObject(GameObject* obj) {
        PlayLayer::addObject(obj);

        if (!listening(HookId::PlayLayerAddObject)) return;

        HookEvent event;
        fillObject(event, obj);
        fire(HookId::PlayLayerAddObject, event);
    }
};

class $modify(NHLuaAllGameLayer, GJBaseGameLayer) {
    void processQueuedButtons(float dt, bool clearInputQueue) {
        if (listening(HookId::GameLayerProcessButtons)) {
            HookEvent event;
            event.setNumber("dt", (double)dt);
            event.setBool("clear_queue", clearInputQueue);
            if (!fire(HookId::GameLayerProcessButtons, event)) return;
        }

        GJBaseGameLayer::processQueuedButtons(dt, clearInputQueue);
    }

    void playerTouchedRing(PlayerObject* player, RingObject* ring) {
        if (listening(HookId::GameLayerTouchedRing)) {
            HookEvent event;
            fillPlayer(event, player);
            fillObject(event, ring);
            if (!fire(HookId::GameLayerTouchedRing, event)) return;
        }

        GJBaseGameLayer::playerTouchedRing(player, ring);
    }

    void playerTouchedTrigger(PlayerObject* player, EffectGameObject* object) {
        if (listening(HookId::GameLayerTouchedTrigger)) {
            HookEvent event;
            fillPlayer(event, player);
            fillObject(event, object);
            if (!fire(HookId::GameLayerTouchedTrigger, event)) return;
        }

        GJBaseGameLayer::playerTouchedTrigger(player, object);
    }

    void gameEventTriggered(GJGameEvent event, int p1, int p2) {
        if (listening(HookId::GameLayerGameEvent)) {
            HookEvent data;
            data.setNumber("event", (double)(int)event);
            data.setNumber("a", (double)p1);
            data.setNumber("b", (double)p2);
            if (!fire(HookId::GameLayerGameEvent, data)) return;
        }

        GJBaseGameLayer::gameEventTriggered(event, p1, p2);
    }

    void shakeCamera(float duration, float strength, float interval) {
        if (listening(HookId::GameLayerShakeCamera)) {
            HookEvent event;
            event.setNumber("duration", (double)duration);
            event.setNumber("strength", (double)strength);
            event.setNumber("interval", (double)interval);
            if (!fire(HookId::GameLayerShakeCamera, event)) return;
        }

        GJBaseGameLayer::shakeCamera(duration, strength, interval);
    }
};

class $modify(NHLuaAllPlayer, PlayerObject) {
    void update(float dt) {
        PlayerObject::update(dt);

        if (!listening(HookId::PlayerUpdate)) return;

        HookEvent event;
        event.setNumber("dt", (double)dt);
        fillPlayer(event, this);
        fire(HookId::PlayerUpdate, event);
    }

    void incrementJumps() {
        if (listening(HookId::PlayerIncrementJumps)) {
            HookEvent event;
            fillPlayer(event, this);
            if (!fire(HookId::PlayerIncrementJumps, event)) return;
        }

        PlayerObject::incrementJumps();
    }

    void playSpiderDashEffect(cocos2d::CCPoint from, cocos2d::CCPoint to) {
        if (listening(HookId::PlayerSpiderDash)) {
            HookEvent event;
            fillPlayer(event, this);
            event.setNumber("from_x", from.x);
            event.setNumber("from_y", from.y);
            event.setNumber("to_x", to.x);
            event.setNumber("to_y", to.y);
            if (!fire(HookId::PlayerSpiderDash, event)) return;
        }

        PlayerObject::playSpiderDashEffect(from, to);
    }

    void ringJump(RingObject* ring, bool p1) {
        if (listening(HookId::PlayerRingJump)) {
            HookEvent event;
            fillPlayer(event, this);
            fillObject(event, ring);
            if (!fire(HookId::PlayerRingJump, event)) return;
        }

        PlayerObject::ringJump(ring, p1);
    }
};

class $modify(NHLuaAllEditor, LevelEditorLayer) {
    void postUpdate(float dt) {
        LevelEditorLayer::postUpdate(dt);

        if (!listening(HookId::EditorPostUpdate)) return;

        HookEvent event;
        event.setNumber("dt", (double)dt);
        fire(HookId::EditorPostUpdate, event);
    }
};

class $modify(NHLuaAllUILayer, UILayer) {
    bool init(GJBaseGameLayer* layer) {
        if (!UILayer::init(layer)) return false;

        if (listening(HookId::UiLayerInit))
            fire(HookId::UiLayerInit);

        return true;
    }
};

class $modify(NHLuaAllKeyboard, cocos2d::CCKeyboardDispatcher) {
    bool dispatchKeyboardMSG(cocos2d::enumKeyCodes key, bool isKeyDown, bool isKeyRepeat,
                             double timestamp) {
        if (listening(HookId::KeyboardMessage)) {
            HookEvent event;
            event.setNumber("key", (double)(int)key);
            event.setBool("down", isKeyDown);
            event.setBool("repeated", isKeyRepeat);
            if (!fire(HookId::KeyboardMessage, event)) return false;
        }

        return cocos2d::CCKeyboardDispatcher::dispatchKeyboardMSG(key, isKeyDown, isKeyRepeat,
                                                                  timestamp);
    }
};

class $modify(NHLuaAllLevelInfo, LevelInfoLayer) {
    void levelDownloadFinished(GJGameLevel* level) {
        LevelInfoLayer::levelDownloadFinished(level);

        if (!listening(HookId::LevelInfoDownloadFinished)) return;

        HookEvent event;
        if (level) {
            event.setNumber("level_id", (double)level->m_levelID.value());
            event.setString("level_name", level->m_levelName.c_str());
        }
        fire(HookId::LevelInfoDownloadFinished, event);
    }

    void onEnterTransitionDidFinish() {
        LevelInfoLayer::onEnterTransitionDidFinish();

        if (listening(HookId::LevelInfoEnterFinished))
            fire(HookId::LevelInfoEnterFinished);
    }
};

class $modify(NHLuaAllCreator, CreatorLayer) {
    bool init() {
        if (!CreatorLayer::init()) return false;

        if (listening(HookId::CreatorLayerInit))
            fire(HookId::CreatorLayerInit);

        return true;
    }
};

class $modify(NHLuaAllGameManager, GameManager) {
    bool init() {
        if (!GameManager::init()) return false;

        if (listening(HookId::GameManagerInit))
            fire(HookId::GameManagerInit);

        return true;
    }
};

class $modify(NHLuaAllAppDelegate, AppDelegate) {
    void applicationWillEnterForeground() {
        AppDelegate::applicationWillEnterForeground();

        if (listening(HookId::AppForeground))
            fire(HookId::AppForeground);
    }
};

class $modify(NHLuaAllAudio, FMODAudioEngine) {
    void fadeOutMusic(float duration, int channel) {
        if (listening(HookId::MusicFadeOut)) {
            HookEvent event;
            event.setNumber("duration", (double)duration);
            event.setNumber("channel", (double)channel);
            if (!fire(HookId::MusicFadeOut, event)) return;
        }

        FMODAudioEngine::fadeOutMusic(duration, channel);
    }
};

class $modify(NHLuaAllObject, GameObject) {
    void playShineEffect() {
        if (listening(HookId::ObjectShine)) {
            HookEvent event;
            fillObject(event, this);
            if (!fire(HookId::ObjectShine, event)) return;
        }

        GameObject::playShineEffect();
    }
};

class $modify(NHLuaAllEffect, EffectGameObject) {
    void triggerObject(GJBaseGameLayer* layer, int p1, const gd::vector<int>* p2) {
        if (listening(HookId::EffectTrigger)) {
            HookEvent event;
            fillObject(event, this);
            event.setNumber("a", (double)p1);
            if (!fire(HookId::EffectTrigger, event)) return;
        }

        EffectGameObject::triggerObject(layer, p1, p2);
    }
};

class $modify(NHLuaAllStreak, HardStreak) {
    void addPoint(cocos2d::CCPoint point) {
        if (listening(HookId::StreakPoint)) {
            HookEvent event;
            event.setNumber("x", point.x);
            event.setNumber("y", point.y);
            if (!fire(HookId::StreakPoint, event)) return;
        }

        HardStreak::addPoint(point);
    }
};
