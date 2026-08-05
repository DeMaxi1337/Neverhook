
#include "LuaEngine.hpp"
#include "LuaHooks.hpp"

#include <Geode/Geode.hpp>
#include <Geode/binding/GJGameLevel.hpp>
#include <Geode/binding/GameObject.hpp>
#include <Geode/binding/PlayLayer.hpp>
#include <Geode/binding/PlayerObject.hpp>
#include <Geode/modify/EditorPauseLayer.hpp>
#include <Geode/modify/EditorUI.hpp>
#include <Geode/modify/EndLevelLayer.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/LevelInfoLayer.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/PlayerObject.hpp>

extern "C" {
#include <lauxlib.h>
#include <lua.h>
}

#include <array>
#include <chrono>
#include <cstring>
#include <string>

using namespace geode::prelude;

namespace nh::lua {

namespace {

char kHooksKey = 0;

constexpr int    kHookCallBudgetMs  = 20;
constexpr double kHookFrameBudgetMs = 8.0;
constexpr int    kMaxOverrunFrames  = 30;

constexpr std::size_t kHookTotal = (std::size_t)HookId::Count;

const std::array<HookDef, kHookTotal> kHookDefs = { {
    { "PlayLayer::init",                    false },
    { "PlayLayer::resetLevel",              false },
    { "PlayLayer::destroyPlayer",           true  },
    { "PlayLayer::levelComplete",           false },
    { "PlayLayer::togglePracticeMode",      false },
    { "GJBaseGameLayer::update",            false },
    { "GJBaseGameLayer::handleButton",      true  },
    { "GJBaseGameLayer::resetLevelVariables", false },
    { "PlayerObject::pushButton",           true  },
    { "PlayerObject::releaseButton",        true  },
    { "PlayerObject::playDeathEffect",      true  },
    { "PlayerObject::playSpawnEffect",      true  },
    { "MenuLayer::init",                    false },
    { "PauseLayer::customSetup",            false },
    { "EditorPauseLayer::customSetup",      false },
    { "EndLevelLayer::customSetup",         false },
    { "LevelInfoLayer::init",               false },
    { "EditorUI::init",                     false },
    { "PlayLayer::postUpdate",              false },
    { "PlayLayer::resetLevelFromStart",     false },
    { "PlayLayer::fullReset",               false },
    { "PlayLayer::pauseGame",               true  },
    { "PlayLayer::onQuit",                  false },
    { "PlayLayer::showCompleteEffect",      true  },
    { "PlayLayer::storeCheckpoint",         true  },
    { "PlayLayer::loadFromCheckpoint",      true  },
    { "PlayLayer::removeCheckpoint",        true  },
    { "PlayLayer::playEndAnimationToPos",   true  },
    { "PlayLayer::addObject",               false },
    { "GJBaseGameLayer::processQueuedButtons", true },
    { "GJBaseGameLayer::playerTouchedRing", true  },
    { "GJBaseGameLayer::playerTouchedTrigger", true },
    { "GJBaseGameLayer::gameEventTriggered", true },
    { "GJBaseGameLayer::shakeCamera",       true  },
    { "PlayerObject::update",               false },
    { "PlayerObject::incrementJumps",       true  },
    { "PlayerObject::playSpiderDashEffect", true  },
    { "PlayerObject::ringJump",             true  },
    { "LevelEditorLayer::postUpdate",       false },
    { "UILayer::init",                      false },
    { "CCKeyboardDispatcher::dispatchKeyboardMSG", true },
    { "LevelInfoLayer::levelDownloadFinished", false },
    { "LevelInfoLayer::onEnterTransitionDidFinish", false },
    { "CreatorLayer::init",                 false },
    { "GameManager::init",                  false },
    { "AppDelegate::applicationWillEnterForeground", false },
    { "FMODAudioEngine::fadeOutMusic",      true  },
    { "GameObject::playShineEffect",        true  },
    { "EffectGameObject::triggerObject",    true  },
    { "HardStreak::addPoint",               true  },
} };

std::array<int, kHookTotal>  g_listeners{};
std::array<bool, kHookTotal> g_inHook{};
bool                         g_listenersDirty = true;

void pushHooksTable(lua_State* L) {
    lua_pushlightuserdata(L, &kHooksKey);
    lua_rawget(L, LUA_REGISTRYINDEX);
}

}

const HookDef& hookDef(HookId id) {
    const std::size_t index = (std::size_t)id;
    return kHookDefs[index < kHookTotal ? index : 0];
}

bool hookFromName(const char* name, HookId& out) {
    if (!name) return false;

    for (std::size_t i = 0; i < kHookTotal; ++i) {
        if (std::strcmp(kHookDefs[i].name, name) == 0) {
            out = (HookId)i;
            return true;
        }
    }
    return false;
}

int hookCount() { return (int)kHookTotal; }

void* hooksRegistryKey() { return &kHooksKey; }

void markHookListenersDirty() { g_listenersDirty = true; }

HookEvent& HookEvent::setNumber(const char* key, double value) {
    Field f;
    f.key    = key;
    f.kind   = Kind::Number;
    f.number = value;
    m_fields.push_back(f);
    return *this;
}

HookEvent& HookEvent::setBool(const char* key, bool value) {
    Field f;
    f.key     = key;
    f.kind    = Kind::Bool;
    f.boolean = value;
    m_fields.push_back(f);
    return *this;
}

HookEvent& HookEvent::setString(const char* key, const std::string& value) {
    Field f;
    f.key  = key;
    f.kind = Kind::String;
    f.text = value;
    m_fields.push_back(f);
    return *this;
}

void HookEvent::pushTable(lua_State* L) const {
    lua_newtable(L);

    for (const Field& f : m_fields) {
        switch (f.kind) {
            case Kind::Number: lua_pushnumber(L, f.number); break;
            case Kind::Bool:   lua_pushboolean(L, f.boolean ? 1 : 0); break;
            case Kind::String: lua_pushlstring(L, f.text.c_str(), f.text.size()); break;
        }
        lua_setfield(L, -2, f.key);
    }
}

bool hookHasListeners(HookId id) {
    if (g_listenersDirty) {
        g_listeners.fill(0);

        for (auto& s : Manager::get().scripts()) {
            if (!s || !s->running()) continue;
            for (std::size_t i = 0; i < kHookTotal; ++i)
                g_listeners[i] += s->hookListenerCount((HookId)i);
        }

        g_listenersDirty = false;
    }

    const std::size_t index = (std::size_t)id;
    return index < kHookTotal && g_listeners[index] > 0;
}

bool dispatchHook(HookId id, const HookEvent& event) {
    const std::size_t index = (std::size_t)id;
    if (index >= kHookTotal) return true;

    if (g_inHook[index]) return true;
    if (!hookHasListeners(id)) return true;

    g_inHook[index] = true;

    bool run = true;
    auto& scripts = Manager::get().scripts();
    for (std::size_t i = 0; i < scripts.size(); ++i) {
        Script* s = scripts[i].get();
        if (!s || !s->running()) continue;
        if (!s->dispatchHook(id, event)) run = false;
    }

    g_inHook[index] = false;
    return run;
}

bool dispatchHook(HookId id) {
    const HookEvent empty;
    return dispatchHook(id, empty);
}

void resetHookFrames() {
    for (auto& s : Manager::get().scripts())
        if (s) s->resetHookFrame();
}

int Script::hookListenerCount(HookId id) const {
    if (!m_L) return 0;

    const int base = lua_gettop(m_L);

    pushHooksTable(m_L);
    if (!lua_istable(m_L, -1)) {
        lua_settop(m_L, base);
        return 0;
    }

    lua_getfield(m_L, -1, hookDef(id).name);
    const int count = lua_istable(m_L, -1) ? (int)lua_objlen(m_L, -1) : 0;

    lua_settop(m_L, base);
    return count;
}

void Script::resetHookFrame() {
    for (HookBudget& b : m_hookBudgets) {
        if (b.usedMs <= kHookFrameBudgetMs) b.overruns = 0;
        b.usedMs = 0.0;
    }
}

bool Script::dispatchHook(HookId id, const HookEvent& event) {
    if (!m_L) return true;

    const std::size_t index = (std::size_t)id;
    if (index >= kHookTotal) return true;

    if (m_hookBudgets.size() != kHookTotal)
        m_hookBudgets.assign(kHookTotal, HookBudget{});

    HookBudget& budget = m_hookBudgets[index];
    if (budget.disabled) return true;

    const int base = lua_gettop(m_L);

    pushHooksTable(m_L);
    if (!lua_istable(m_L, -1)) {
        lua_settop(m_L, base);
        return true;
    }

    lua_getfield(m_L, -1, hookDef(id).name);
    if (!lua_istable(m_L, -1)) {
        lua_settop(m_L, base);
        return true;
    }

    const int list  = lua_gettop(m_L);
    const int count = (int)lua_objlen(m_L, list);
    if (count == 0) {
        lua_settop(m_L, base);
        return true;
    }

    const auto started = std::chrono::steady_clock::now();
    bool       run     = true;

    for (int i = 1; i <= count; ++i) {
        lua_rawgeti(m_L, list, i);
        if (!lua_isfunction(m_L, -1)) {
            lua_pop(m_L, 1);
            continue;
        }

        event.pushTable(m_L);

        beginBudget(kHookCallBudgetMs);
        if (!pcall(1, 1, hookDef(id).name)) {

            afterCallbackError();
            markHookListenersDirty();
            return true;
        }

        if (lua_isboolean(m_L, -1) && !lua_toboolean(m_L, -1)) {
            if (hookDef(id).cancelable) {
                run = false;
            }
            else if (!budget.warned) {
                budget.warned = true;
                Manager::get().log(
                    m_fileName + ": " + hookDef(id).name +
                        " cannot be cancelled, the returned false was ignored",
                    true);
            }
        }

        lua_pop(m_L, 1);
    }

    lua_settop(m_L, base);

    const auto spent = std::chrono::steady_clock::now() - started;
    budget.usedMs += std::chrono::duration<double, std::milli>(spent).count();

    if (budget.usedMs > kHookFrameBudgetMs && ++budget.overruns >= kMaxOverrunFrames) {
        budget.disabled = true;
        Manager::get().log(
            m_fileName + ": " + hookDef(id).name +
                " is too slow, this hook was switched off for this script"
                " (restart the script to try again)",
            true);
    }

    return run;
}

}

using nh::lua::dispatchHook;
using nh::lua::HookEvent;
using nh::lua::HookId;
using nh::lua::hookHasListeners;

namespace {

bool nhIsPlayerTwo(PlayerObject* player) {
    auto* pl = PlayLayer::get();
    return pl && player && player == pl->m_player2;
}

void nhFillLevel(HookEvent& event) {
    auto* pl = PlayLayer::get();
    if (!pl) return;

    event.setNumber("percent", pl->getCurrentPercent());
    event.setBool("practice", pl->m_isPracticeMode);

    if (auto* lvl = pl->m_level) {
        event.setNumber("level_id", (double)lvl->m_levelID.value());
        event.setString("level_name", lvl->m_levelName.c_str());
        event.setNumber("attempts", (double)lvl->m_attempts.value());
    }
}

}

class $modify(NHLuaHookPlayLayer, PlayLayer) {
    bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
        if (!PlayLayer::init(level, useReplay, dontCreateObjects)) return false;

        if (hookHasListeners(HookId::PlayLayerInit)) {
            HookEvent event;
            if (level) {
                event.setNumber("level_id", (double)level->m_levelID.value());
                event.setString("level_name", level->m_levelName.c_str());
                event.setBool("platformer", level->isPlatformer());
            }
            dispatchHook(HookId::PlayLayerInit, event);
        }

        return true;
    }

    void resetLevel() {
        PlayLayer::resetLevel();

        if (!hookHasListeners(HookId::PlayLayerReset)) return;

        HookEvent event;
        nhFillLevel(event);
        dispatchHook(HookId::PlayLayerReset, event);
    }

    void destroyPlayer(PlayerObject* player, GameObject* object) {
        if (hookHasListeners(HookId::PlayLayerDestroyPlayer)) {
            HookEvent event;
            nhFillLevel(event);
            event.setBool("player2", nhIsPlayerTwo(player));
            event.setNumber("object_id", object ? (double)object->m_objectID : -1.0);

            if (!dispatchHook(HookId::PlayLayerDestroyPlayer, event)) return;
        }

        PlayLayer::destroyPlayer(player, object);
    }

    void levelComplete() {
        if (hookHasListeners(HookId::PlayLayerLevelComplete)) {
            HookEvent event;
            nhFillLevel(event);
            dispatchHook(HookId::PlayLayerLevelComplete, event);
        }

        PlayLayer::levelComplete();
    }

    void togglePracticeMode(bool practice) {
        PlayLayer::togglePracticeMode(practice);

        if (!hookHasListeners(HookId::PlayLayerTogglePractice)) return;

        HookEvent event;
        event.setBool("practice", practice);
        dispatchHook(HookId::PlayLayerTogglePractice, event);
    }
};

class $modify(NHLuaHookGameLayer, GJBaseGameLayer) {
    void update(float dt) {
        GJBaseGameLayer::update(dt);

        if (!hookHasListeners(HookId::GameLayerUpdate)) return;

        HookEvent event;
        event.setNumber("dt", (double)dt);
        dispatchHook(HookId::GameLayerUpdate, event);
    }

    void handleButton(bool down, int button, bool player1) {
        if (hookHasListeners(HookId::GameLayerHandleButton)) {
            HookEvent event;
            event.setBool("down", down);
            event.setNumber("button", (double)button);
            event.setBool("player1", player1);

            if (!dispatchHook(HookId::GameLayerHandleButton, event)) return;
        }

        GJBaseGameLayer::handleButton(down, button, player1);
    }

    void resetLevelVariables() {
        GJBaseGameLayer::resetLevelVariables();
        dispatchHook(HookId::GameLayerResetVariables);
    }
};

class $modify(NHLuaHookPlayer, PlayerObject) {
    void pushButton(PlayerButton button) {
        if (hookHasListeners(HookId::PlayerPushButton)) {
            HookEvent event;
            event.setNumber("button", (double)(int)button);
            event.setBool("player2", nhIsPlayerTwo(this));

            if (!dispatchHook(HookId::PlayerPushButton, event)) return;
        }

        PlayerObject::pushButton(button);
    }

    void releaseButton(PlayerButton button) {
        if (hookHasListeners(HookId::PlayerReleaseButton)) {
            HookEvent event;
            event.setNumber("button", (double)(int)button);
            event.setBool("player2", nhIsPlayerTwo(this));

            if (!dispatchHook(HookId::PlayerReleaseButton, event)) return;
        }

        PlayerObject::releaseButton(button);
    }

    void playDeathEffect() {
        if (hookHasListeners(HookId::PlayerDeathEffect)) {
            HookEvent event;
            event.setBool("player2", nhIsPlayerTwo(this));

            if (!dispatchHook(HookId::PlayerDeathEffect, event)) return;
        }

        PlayerObject::playDeathEffect();
    }

    void playSpawnEffect() {
        if (hookHasListeners(HookId::PlayerSpawnEffect)) {
            HookEvent event;
            event.setBool("player2", nhIsPlayerTwo(this));

            if (!dispatchHook(HookId::PlayerSpawnEffect, event)) return;
        }

        PlayerObject::playSpawnEffect();
    }
};

class $modify(NHLuaHookMenuLayer, MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) return false;
        dispatchHook(HookId::MenuLayerInit);
        return true;
    }
};

class $modify(NHLuaHookPauseLayer, PauseLayer) {
    void customSetup() {
        PauseLayer::customSetup();
        dispatchHook(HookId::PauseLayerSetup);
    }
};

class $modify(NHLuaHookEditorPauseLayer, EditorPauseLayer) {
    void customSetup() {
        EditorPauseLayer::customSetup();
        dispatchHook(HookId::EditorPauseLayerSetup);
    }
};

class $modify(NHLuaHookEndLevelLayer, EndLevelLayer) {
    void customSetup() {
        EndLevelLayer::customSetup();
        dispatchHook(HookId::EndLevelLayerSetup);
    }
};

class $modify(NHLuaHookLevelInfoLayer, LevelInfoLayer) {
    bool init(GJGameLevel* level, bool challenge) {
        if (!LevelInfoLayer::init(level, challenge)) return false;

        if (hookHasListeners(HookId::LevelInfoLayerInit)) {
            HookEvent event;
            if (level) {
                event.setNumber("level_id", (double)level->m_levelID.value());
                event.setString("level_name", level->m_levelName.c_str());
            }
            event.setBool("challenge", challenge);
            dispatchHook(HookId::LevelInfoLayerInit, event);
        }

        return true;
    }
};

class $modify(NHLuaHookEditorUI, EditorUI) {
    bool init(LevelEditorLayer* editorLayer) {
        if (!EditorUI::init(editorLayer)) return false;
        dispatchHook(HookId::EditorUiInit);
        return true;
    }
};
