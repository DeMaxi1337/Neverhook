
#include "LuaEngine.hpp"

#include <Geode/Geode.hpp>
#include <Geode/binding/GJBaseGameLayer.hpp>
#include <Geode/binding/GameManager.hpp>
#include <Geode/binding/LevelEditorLayer.hpp>
#include <Geode/binding/PlayLayer.hpp>
#include <Geode/binding/PlayerObject.hpp>

extern "C" {
#include <lauxlib.h>
#include <lua.h>
}

#include <concepts>
#include <cstring>
#include <string>

using namespace geode::prelude;

namespace nh::lua {
namespace {

template <class T>
concept HasShakeFields = requires(T* layer) {
    layer->m_gameState.m_cameraShakeEnabled;
    layer->m_gameState.m_cameraShakeFactor;
};

template <class T>
concept HasLevelTime = requires(T* layer) { layer->m_gameState.m_levelTime; };

template <class T>
concept HasProgress = requires(T* layer) { layer->m_gameState.m_currentProgress; };

template <class T>
concept HasObjects = requires(T* layer) { layer->m_objects; };

template <class T>
concept HasStartPos = requires(T* layer) { layer->m_startPosObject; };

template <class T>
concept HasEndAnimation = requires(T* layer) { layer->m_levelEndAnimationStarted; };

template <class T>
concept HasSelected = requires(T* layer) { layer->m_selectedObjects; };

template <class T>
concept HasPlayerName = requires(T* manager) { manager->m_playerName; };

template <class T>
concept HasPracticeFlag = requires(T* layer) { layer->m_isPracticeMode; };

template <class T>
concept HasPausedFlag = requires(T* layer) { layer->m_isPaused; };

template <class T>
void setShake(T* layer, bool enabled, float factor) {
    if constexpr (HasShakeFields<T>) {
        layer->m_gameState.m_cameraShakeEnabled = enabled;
        layer->m_gameState.m_cameraShakeFactor  = factor;
    }
}

template <class T>
double levelTimeOf(T* layer) {
    if constexpr (HasLevelTime<T>) return (double)layer->m_gameState.m_levelTime;
    else                           return 0.0;
}

template <class T>
double progressOf(T* layer) {
    if constexpr (HasProgress<T>) return (double)layer->m_gameState.m_currentProgress;
    else                          return 0.0;
}

template <class T>
int objectCountOf(T* layer) {
    if constexpr (HasObjects<T>) {
        return layer->m_objects ? (int)layer->m_objects->count() : 0;
    } else {
        return 0;
    }
}

template <class T>
bool hasStartPosOf(T* layer) {
    if constexpr (HasStartPos<T>) return layer->m_startPosObject != nullptr;
    else                          return false;
}

template <class T>
bool endAnimationOf(T* layer) {
    if constexpr (HasEndAnimation<T>) return layer->m_levelEndAnimationStarted;
    else                              return false;
}

template <class T>
int selectedCountOf(T* layer) {
    if constexpr (HasSelected<T>) {
        return layer->m_selectedObjects ? (int)layer->m_selectedObjects->count() : 0;
    } else {
        return 0;
    }
}

template <class T>
std::string playerNameOf(T* manager) {
    if constexpr (HasPlayerName<T>) return manager->m_playerName;
    else                            return std::string();
}

template <class T>
bool practiceOf(T* layer) {
    if constexpr (HasPracticeFlag<T>) return layer->m_isPracticeMode;
    else                              return false;
}

template <class T>
bool pausedOf(T* layer) {
    if constexpr (HasPausedFlag<T>) return layer->m_isPaused;
    else                            return false;
}

int l_world_in_level(lua_State* L) {
    lua_pushboolean(L, PlayLayer::get() != nullptr ? 1 : 0);
    return 1;
}

int l_world_in_editor(lua_State* L) {
    lua_pushboolean(L, LevelEditorLayer::get() != nullptr ? 1 : 0);
    return 1;
}

int l_world_reset(lua_State* L) {
    if (auto* layer = PlayLayer::get()) layer->resetLevel();
    return 0;
}

int l_world_reset_from_start(lua_State* L) {
    if (auto* layer = PlayLayer::get()) layer->resetLevelFromStart();
    return 0;
}

int l_world_full_reset(lua_State* L) {
    if (auto* layer = PlayLayer::get()) layer->fullReset();
    return 0;
}

int l_world_kill(lua_State* L) {
    auto* layer = PlayLayer::get();
    if (!layer) return 0;

    PlayerObject* player = layer->m_player1;
    if (!player) return 0;

    layer->destroyPlayer(player, nullptr);
    return 0;
}

int l_world_pause(lua_State* L) {
    auto* layer = PlayLayer::get();
    if (!layer) return 0;

    if (!layer->canPauseGame()) {
        lua_pushboolean(L, 0);
        return 1;
    }

    layer->pauseGame(lua_toboolean(L, 1) != 0);
    lua_pushboolean(L, 1);
    return 1;
}

int l_world_resume(lua_State* L) {
    if (auto* layer = PlayLayer::get()) layer->resume();
    return 0;
}

int l_world_restart(lua_State* L) {
    const bool fromStart = lua_toboolean(L, 1) != 0;

    if (auto* layer = PlayLayer::get()) layer->resumeAndRestart(fromStart);
    return 0;
}

int l_world_can_pause(lua_State* L) {
    auto* layer = PlayLayer::get();
    lua_pushboolean(L, (layer && layer->canPauseGame()) ? 1 : 0);
    return 1;
}

int l_world_is_paused(lua_State* L) {
    auto* layer = PlayLayer::get();
    lua_pushboolean(L, (layer && pausedOf(layer)) ? 1 : 0);
    return 1;
}

int l_world_set_practice(lua_State* L) {
    const bool practice = lua_toboolean(L, 1) != 0;

    if (auto* layer = PlayLayer::get()) layer->togglePracticeMode(practice);
    return 0;
}

int l_world_is_practice(lua_State* L) {
    auto* layer = PlayLayer::get();
    lua_pushboolean(L, (layer && practiceOf(layer)) ? 1 : 0);
    return 1;
}

int l_world_process_checkpoints(lua_State* L) {
    if (auto* layer = PlayLayer::get()) layer->processCheckpoints();
    return 0;
}

int l_world_toggle_ground(lua_State* L) {
    const bool visible = lua_toboolean(L, 1) != 0;

    if (auto* layer = PlayLayer::get()) layer->toggleGroundVisibility(visible);
    return 0;
}

int l_world_toggle_practice_music(lua_State* L) {
    if (auto* layer = PlayLayer::get()) layer->toggleMusicInPractice();
    return 0;
}

int l_world_complete(lua_State* L) {
    auto* layer = PlayLayer::get();
    if (!layer) return 0;

    Script* script = ownerOf(L);
    Manager::get().log(
        (script ? script->fileName() : std::string("script")) + ": completed the level");

    layer->levelComplete();
    return 0;
}

int l_world_time(lua_State* L) {
    auto* layer = GJBaseGameLayer::get();
    lua_pushnumber(L, layer ? levelTimeOf(layer) : 0.0);
    return 1;
}

int l_world_progress(lua_State* L) {
    auto* layer = GJBaseGameLayer::get();
    lua_pushnumber(L, layer ? progressOf(layer) : 0.0);
    return 1;
}

int l_world_object_count(lua_State* L) {
    auto* layer = GJBaseGameLayer::get();
    lua_pushinteger(L, layer ? objectCountOf(layer) : 0);
    return 1;
}

int l_world_selected_count(lua_State* L) {
    auto* editor = LevelEditorLayer::get();
    lua_pushinteger(L, editor ? selectedCountOf(editor) : 0);
    return 1;
}

int l_world_has_start_pos(lua_State* L) {
    auto* layer = PlayLayer::get();
    lua_pushboolean(L, (layer && hasStartPosOf(layer)) ? 1 : 0);
    return 1;
}

int l_world_ending(lua_State* L) {
    auto* layer = PlayLayer::get();
    lua_pushboolean(L, (layer && endAnimationOf(layer)) ? 1 : 0);
    return 1;
}

int l_world_shake(lua_State* L) {
    auto* layer = GJBaseGameLayer::get();
    if (!layer) return 0;

    const bool  enabled = lua_toboolean(L, 1) != 0;
    float       factor  = (float)luaL_optnumber(L, 2, 1.0);

    if (factor < 0.f)  factor = 0.f;
    if (factor > 10.f) factor = 10.f;

    setShake(layer, enabled, factor);
    return 0;
}

int l_world_player_name(lua_State* L) {
    auto* manager = GameManager::sharedState();
    if (!manager) { lua_pushnil(L); return 1; }

    const std::string name = playerNameOf(manager);
    if (name.empty()) { lua_pushnil(L); return 1; }

    lua_pushlstring(L, name.data(), name.size());
    return 1;
}

int l_world_game_variable(lua_State* L) {
    const char* key = luaL_checkstring(L, 1);

    if (std::strlen(key) > 8)
        return luaL_error(L, "that is not a game variable key");

    auto* manager = GameManager::sharedState();
    if (!manager) { lua_pushnil(L); return 1; }

    lua_pushboolean(L, manager->getGameVariable(key) ? 1 : 0);
    return 1;
}

int l_world_screen_size(lua_State* L) {
    auto* director = CCDirector::sharedDirector();
    if (!director) return 0;

    const CCSize size = director->getWinSize();
    lua_pushnumber(L, size.width);
    lua_pushnumber(L, size.height);
    return 2;
}

int l_world_scene_name(lua_State* L) {
    if (PlayLayer::get())        { lua_pushstring(L, "play");   return 1; }
    if (LevelEditorLayer::get()) { lua_pushstring(L, "editor"); return 1; }

    lua_pushstring(L, "menu");
    return 1;
}

void addFunctions(lua_State* L, const char* global, const luaL_Reg* fns) {
    lua_getglobal(L, global);

    if (!lua_istable(L, -1)) {
        lua_pop(L, 1);
        lua_newtable(L);
        lua_pushvalue(L, -1);
        lua_setglobal(L, global);
    }

    for (const luaL_Reg* f = fns; f->name; ++f) {
        lua_pushcfunction(L, f->func);
        lua_setfield(L, -2, f->name);
    }

    lua_pop(L, 1);
}

}

void registerWorldApi(lua_State* L) {
    static const luaL_Reg kWorld[] = {

        { "in_level",               l_world_in_level               },
        { "in_editor",              l_world_in_editor              },
        { "scene_name",             l_world_scene_name             },
        { "screen_size",            l_world_screen_size            },

        { "reset",                  l_world_reset                  },
        { "reset_from_start",       l_world_reset_from_start        },
        { "full_reset",             l_world_full_reset             },
        { "kill",                   l_world_kill                   },
        { "pause",                  l_world_pause                  },
        { "resume",                 l_world_resume                 },
        { "restart",                l_world_restart                },
        { "can_pause",              l_world_can_pause              },
        { "is_paused",              l_world_is_paused              },
        { "set_practice",           l_world_set_practice           },
        { "is_practice",            l_world_is_practice            },
        { "process_checkpoints",    l_world_process_checkpoints    },
        { "toggle_ground",          l_world_toggle_ground          },
        { "toggle_practice_music",  l_world_toggle_practice_music  },
        { "complete",               l_world_complete               },

        { "time",                   l_world_time                   },
        { "progress",               l_world_progress               },
        { "object_count",           l_world_object_count           },
        { "selected_count",         l_world_selected_count         },
        { "has_start_pos",          l_world_has_start_pos          },
        { "ending",                 l_world_ending                 },
        { "shake",                  l_world_shake                  },

        { "player_name",            l_world_player_name            },
        { "game_variable",          l_world_game_variable          },
        { nullptr,                  nullptr                        },
    };

    lua_newtable(L);
    lua_setglobal(L, "world");
    addFunctions(L, "world", kWorld);
}

}
