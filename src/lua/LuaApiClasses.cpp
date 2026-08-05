
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

#include <cstring>
#include <functional>
#include <string>
#include <type_traits>
#include <vector>

using namespace geode::prelude;

namespace nh::lua {
namespace {

constexpr int kMaxTextArg = 128;

struct FieldDef {
    const char* name     = "";
    const char* type     = "number";
    bool        writable = true;

    std::function<void(lua_State*, void*)>      push;
    std::function<void(lua_State*, int, void*)> assign;
};

struct MethodDef {
    const char* name = "";
    const char* args = "";

    std::function<int(lua_State*, void*)> call;
};

struct ClassDef {
    const char*             name = "";
    std::function<void*()>  resolve;
    std::vector<FieldDef>   fields;
    std::vector<MethodDef>  methods;
};

template <class T, class M>
FieldDef field(const char* name, M T::*member, bool writable = true) {
    FieldDef def;
    def.name     = name;
    def.writable = writable;
    def.type     = std::is_same_v<M, bool> ? "boolean" : "number";

    def.push = [member](lua_State* L, void* instance) {
        T* object = static_cast<T*>(instance);
        if constexpr (std::is_same_v<M, bool>)
            lua_pushboolean(L, (object->*member) ? 1 : 0);
        else
            lua_pushnumber(L, (double)(object->*member));
    };

    def.assign = [member](lua_State* L, int index, void* instance) {
        T* object = static_cast<T*>(instance);
        if constexpr (std::is_same_v<M, bool>)
            object->*member = lua_toboolean(L, index) != 0;
        else
            object->*member = (M)luaL_checknumber(L, index);
    };

    return def;
}

template <class T>
MethodDef method0(const char* name, void (T::*fn)()) {
    MethodDef def;
    def.name = name;
    def.call = [fn](lua_State*, void* instance) {
        (static_cast<T*>(instance)->*fn)();
        return 0;
    };
    return def;
}

template <class T>
MethodDef method1b(const char* name, void (T::*fn)(bool)) {
    MethodDef def;
    def.name = name;
    def.args = "boolean";
    def.call = [fn](lua_State* L, void* instance) {
        (static_cast<T*>(instance)->*fn)(lua_toboolean(L, 3) != 0);
        return 0;
    };
    return def;
}

template <class T>
MethodDef query0(const char* name, bool (T::*fn)()) {
    MethodDef def;
    def.name = name;
    def.call = [fn](lua_State* L, void* instance) {
        lua_pushboolean(L, (static_cast<T*>(instance)->*fn)() ? 1 : 0);
        return 1;
    };
    return def;
}

PlayLayer*        playLayer()   { return PlayLayer::get(); }
GJBaseGameLayer*  gameLayer()   { return GJBaseGameLayer::get(); }
LevelEditorLayer* editorLayer() { return LevelEditorLayer::get(); }

PlayerObject* playerAt(int index) {
    GJBaseGameLayer* game = gameLayer();
    if (!game) return nullptr;
    return index == 2 ? game->m_player2 : game->m_player1;
}

std::vector<FieldDef> playerFields() {
    std::vector<FieldDef> list;

    list.push_back(field("m_yVelocity", &PlayerObject::m_yVelocity));
    list.push_back(field("m_fallSpeed", &PlayerObject::m_fallSpeed));
    list.push_back(field("m_gravity", &PlayerObject::m_gravity));
    list.push_back(field("m_gravityMod", &PlayerObject::m_gravityMod));
    list.push_back(field("m_speedMultiplier", &PlayerObject::m_speedMultiplier));
    list.push_back(field("m_vehicleSize", &PlayerObject::m_vehicleSize));
    list.push_back(field("m_isDead", &PlayerObject::m_isDead));
    list.push_back(field("m_isOnGround", &PlayerObject::m_isOnGround));
    list.push_back(field("m_isUpsideDown", &PlayerObject::m_isUpsideDown));
    list.push_back(field("m_isOnSlope", &PlayerObject::m_isOnSlope));
    list.push_back(field("m_wasOnSlope", &PlayerObject::m_wasOnSlope));
    list.push_back(field("m_slopeVelocity", &PlayerObject::m_slopeVelocity));
    list.push_back(field("m_slopeAngle", &PlayerObject::m_slopeAngle));
    list.push_back(field("m_slopeStartTime", &PlayerObject::m_slopeStartTime));
    list.push_back(field("m_isCollidingWithSlope", &PlayerObject::m_isCollidingWithSlope));
    list.push_back(field("m_yVelocityBeforeSlope", &PlayerObject::m_yVelocityBeforeSlope));
    list.push_back(field("m_dashX", &PlayerObject::m_dashX));
    list.push_back(field("m_dashY", &PlayerObject::m_dashY));
    list.push_back(field("m_dashAngle", &PlayerObject::m_dashAngle));
    list.push_back(field("m_dashStartTime", &PlayerObject::m_dashStartTime));
    list.push_back(field("m_jumpBuffered", &PlayerObject::m_jumpBuffered));
    list.push_back(field("m_stateRingJump", &PlayerObject::m_stateRingJump));
    list.push_back(field("m_wasTeleported", &PlayerObject::m_wasTeleported));
    list.push_back(field("m_fixGravityBug", &PlayerObject::m_fixGravityBug));
    list.push_back(field("m_lastCollisionTop", &PlayerObject::m_lastCollisionTop, false));
    list.push_back(field("m_lastCollisionBottom", &PlayerObject::m_lastCollisionBottom, false));
    list.push_back(field("m_lastCollisionLeft", &PlayerObject::m_lastCollisionLeft, false));
    list.push_back(field("m_lastCollisionRight", &PlayerObject::m_lastCollisionRight, false));

    return list;
}

std::vector<MethodDef> playerMethods() {
    std::vector<MethodDef> list;

    MethodDef push;
    push.name = "pushButton";
    push.args = "button";
    push.call = [](lua_State* L, void* instance) {
        const int button = (int)luaL_optinteger(L, 3, 1);
        static_cast<PlayerObject*>(instance)->pushButton((PlayerButton)button);
        return 0;
    };
    list.push_back(push);

    MethodDef release;
    release.name = "releaseButton";
    release.args = "button";
    release.call = [](lua_State* L, void* instance) {
        const int button = (int)luaL_optinteger(L, 3, 1);
        static_cast<PlayerObject*>(instance)->releaseButton((PlayerButton)button);
        return 0;
    };
    list.push_back(release);

    list.push_back(method0("playDeathEffect", &PlayerObject::playDeathEffect));
    list.push_back(method0("incrementJumps", &PlayerObject::incrementJumps));

    return list;
}

const std::vector<ClassDef>& classes() {
    static std::vector<ClassDef> list = [] {
        std::vector<ClassDef> all;

        ClassDef play;
        play.name    = "PlayLayer";
        play.resolve = []() -> void* { return playLayer(); };
        play.fields.push_back(field("m_isPaused", &PlayLayer::m_isPaused));
        play.fields.push_back(field("m_pauseDelta", &PlayLayer::m_pauseDelta));
        play.fields.push_back(field("m_isPracticeMode", &PlayLayer::m_isPracticeMode));
        play.fields.push_back(
            field("m_levelEndAnimationStarted", &PlayLayer::m_levelEndAnimationStarted, false));
        play.methods.push_back(method0("resetLevel", &PlayLayer::resetLevel));
        play.methods.push_back(method0("resetLevelFromStart", &PlayLayer::resetLevelFromStart));
        play.methods.push_back(method0("fullReset", &PlayLayer::fullReset));
        play.methods.push_back(method0("delayedResetLevel", &PlayLayer::delayedResetLevel));
        play.methods.push_back(method0("delayedFullReset", &PlayLayer::delayedFullReset));
        play.methods.push_back(method0("resume", &PlayLayer::resume));
        play.methods.push_back(method0("processCheckpoints", &PlayLayer::processCheckpoints));
        play.methods.push_back(method0("levelComplete", &PlayLayer::levelComplete));
        play.methods.push_back(method0("onQuit", &PlayLayer::onQuit));
        play.methods.push_back(method1b("pauseGame", &PlayLayer::pauseGame));
        play.methods.push_back(method1b("resumeAndRestart", &PlayLayer::resumeAndRestart));
        play.methods.push_back(method1b("togglePracticeMode", &PlayLayer::togglePracticeMode));
        play.methods.push_back(
            method1b("toggleGroundVisibility", &PlayLayer::toggleGroundVisibility));
        play.methods.push_back(query0("canPauseGame", &PlayLayer::canPauseGame));
        all.push_back(play);

        ClassDef game;
        game.name    = "GJBaseGameLayer";
        game.resolve = []() -> void* { return gameLayer(); };
        game.fields.push_back(field("m_clickBetweenSteps", &GJBaseGameLayer::m_clickBetweenSteps));
        game.fields.push_back(field("m_clickOnSteps", &GJBaseGameLayer::m_clickOnSteps));
        game.methods.push_back(
            method0("resetLevelVariables", &GJBaseGameLayer::resetLevelVariables));

        MethodDef handle;
        handle.name = "handleButton";
        handle.args = "down, button, player1";
        handle.call = [](lua_State* L, void* instance) {
            const bool down    = lua_toboolean(L, 3) != 0;
            const int  button  = (int)luaL_optinteger(L, 4, 1);
            const bool player1 = lua_isnoneornil(L, 5) ? true : lua_toboolean(L, 5) != 0;
            static_cast<GJBaseGameLayer*>(instance)->handleButton(down, button, player1);
            return 0;
        };
        game.methods.push_back(handle);

        MethodDef queued;
        queued.name = "processQueuedButtons";
        queued.args = "dt, clearQueue";
        queued.call = [](lua_State* L, void* instance) {
            const float dt    = (float)luaL_optnumber(L, 3, 0.0);
            const bool  clear = lua_toboolean(L, 4) != 0;
            static_cast<GJBaseGameLayer*>(instance)->processQueuedButtons(dt, clear);
            return 0;
        };
        game.methods.push_back(queued);

        MethodDef shake;
        shake.name = "shakeCamera";
        shake.args = "duration, strength, interval";
        shake.call = [](lua_State* L, void* instance) {
            const float duration = (float)luaL_optnumber(L, 3, 0.5);
            const float strength = (float)luaL_optnumber(L, 4, 1.0);
            const float interval = (float)luaL_optnumber(L, 5, 0.05);
            static_cast<GJBaseGameLayer*>(instance)->shakeCamera(duration, strength, interval);
            return 0;
        };
        game.methods.push_back(shake);
        all.push_back(game);

        ClassDef state;
        state.name    = "GJGameState";
        state.resolve = []() -> void* {
            GJBaseGameLayer* layer = gameLayer();
            return layer ? (void*)&layer->m_gameState : nullptr;
        };
        state.fields.push_back(field("m_currentProgress", &GJGameState::m_currentProgress));
        state.fields.push_back(field("m_levelTime", &GJGameState::m_levelTime));
        state.fields.push_back(field("m_timeWarp", &GJGameState::m_timeWarp));
        state.fields.push_back(field("m_isDualMode", &GJGameState::m_isDualMode));
        state.fields.push_back(
            field("m_cameraShakeEnabled", &GJGameState::m_cameraShakeEnabled));
        state.fields.push_back(field("m_cameraShakeFactor", &GJGameState::m_cameraShakeFactor));
        all.push_back(state);

        ClassDef player1;
        player1.name    = "PlayerObject";
        player1.resolve = []() -> void* { return playerAt(1); };
        player1.fields  = playerFields();
        player1.methods = playerMethods();
        all.push_back(player1);

        ClassDef player2 = player1;
        player2.name     = "PlayerObject2";
        player2.resolve  = []() -> void* { return playerAt(2); };
        all.push_back(player2);

        ClassDef editor;
        editor.name    = "LevelEditorLayer";
        editor.resolve = []() -> void* { return editorLayer(); };
        all.push_back(editor);

        ClassDef manager;
        manager.name    = "GameManager";
        manager.resolve = []() -> void* { return GameManager::sharedState(); };

        MethodDef getVar;
        getVar.name = "getGameVariable";
        getVar.args = "key";
        getVar.call = [](lua_State* L, void* instance) {
            const char* key = luaL_checkstring(L, 3);
            if (!key || std::strlen(key) > kMaxTextArg)
                return luaL_error(L, "that game variable key is not valid");

            lua_pushboolean(
                L, static_cast<GameManager*>(instance)->getGameVariable(key) ? 1 : 0);
            return 1;
        };
        manager.methods.push_back(getVar);

        MethodDef setVar;
        setVar.name = "setGameVariable";
        setVar.args = "key, value";
        setVar.call = [](lua_State* L, void* instance) {
            const char* key = luaL_checkstring(L, 3);
            if (!key || std::strlen(key) > kMaxTextArg)
                return luaL_error(L, "that game variable key is not valid");

            static_cast<GameManager*>(instance)->setGameVariable(key,
                                                                lua_toboolean(L, 4) != 0);
            return 0;
        };
        manager.methods.push_back(setVar);
        all.push_back(manager);

        return all;
    }();

    return list;
}

const ClassDef* classByName(const char* name) {
    if (!name) return nullptr;

    for (const ClassDef& def : classes())
        if (std::strcmp(def.name, name) == 0) return &def;

    return nullptr;
}

const ClassDef* classArg(lua_State* L) {
    const char*     name = luaL_checkstring(L, 1);
    const ClassDef* def  = classByName(name);

    if (!def) luaL_error(L, "unknown class '%s' (see cls.list())", name ? name : "");

    return def;
}

const FieldDef* fieldByName(const ClassDef& def, const char* name) {
    if (!name) return nullptr;

    for (const FieldDef& f : def.fields)
        if (std::strcmp(f.name, name) == 0) return &f;

    return nullptr;
}

const MethodDef* methodByName(const ClassDef& def, const char* name) {
    if (!name) return nullptr;

    for (const MethodDef& m : def.methods)
        if (std::strcmp(m.name, name) == 0) return &m;

    return nullptr;
}

int l_cls_list(lua_State* L) {
    lua_newtable(L);

    int index = 1;
    for (const ClassDef& def : classes()) {
        lua_pushstring(L, def.name);
        lua_rawseti(L, -2, index++);
    }

    return 1;
}

int l_cls_exists(lua_State* L) {
    const ClassDef* def = classArg(L);

    lua_pushboolean(L, def && def->resolve && def->resolve() != nullptr);
    return 1;
}

int l_cls_fields(lua_State* L) {
    const ClassDef* def = classArg(L);

    lua_newtable(L);

    int index = 1;
    for (const FieldDef& f : def->fields) {
        lua_newtable(L);

        lua_pushstring(L, f.name);
        lua_setfield(L, -2, "name");

        lua_pushstring(L, f.type);
        lua_setfield(L, -2, "type");

        lua_pushboolean(L, f.writable ? 1 : 0);
        lua_setfield(L, -2, "writable");

        lua_rawseti(L, -2, index++);
    }

    return 1;
}

int l_cls_methods(lua_State* L) {
    const ClassDef* def = classArg(L);

    lua_newtable(L);

    int index = 1;
    for (const MethodDef& m : def->methods) {
        lua_newtable(L);

        lua_pushstring(L, m.name);
        lua_setfield(L, -2, "name");

        lua_pushstring(L, m.args);
        lua_setfield(L, -2, "args");

        lua_rawseti(L, -2, index++);
    }

    return 1;
}

int l_cls_get(lua_State* L) {
    const ClassDef* def  = classArg(L);
    const char*     name = luaL_checkstring(L, 2);

    const FieldDef* f = fieldByName(*def, name);
    if (!f)
        return luaL_error(L, "'%s' has no field '%s' (see cls.fields())", def->name,
                          name ? name : "");

    void* instance = def->resolve ? def->resolve() : nullptr;
    if (!instance) {
        lua_pushnil(L);
        return 1;
    }

    f->push(L, instance);
    return 1;
}

int l_cls_set(lua_State* L) {
    const ClassDef* def  = classArg(L);
    const char*     name = luaL_checkstring(L, 2);

    const FieldDef* f = fieldByName(*def, name);
    if (!f)
        return luaL_error(L, "'%s' has no field '%s' (see cls.fields())", def->name,
                          name ? name : "");

    if (!f->writable)
        return luaL_error(L, "'%s.%s' is read only", def->name, f->name);

    void* instance = def->resolve ? def->resolve() : nullptr;
    if (!instance) {
        lua_pushboolean(L, 0);
        return 1;
    }

    f->assign(L, 3, instance);

    lua_pushboolean(L, 1);
    return 1;
}

int l_cls_call(lua_State* L) {
    const ClassDef* def  = classArg(L);
    const char*     name = luaL_checkstring(L, 2);

    const MethodDef* m = methodByName(*def, name);
    if (!m)
        return luaL_error(L, "'%s' has no method '%s' (see cls.methods())", def->name,
                          name ? name : "");

    void* instance = def->resolve ? def->resolve() : nullptr;
    if (!instance) {
        lua_pushnil(L);
        return 1;
    }

    return m->call(L, instance);
}

}

void registerClassApi(lua_State* L) {
    static const luaL_Reg kCls[] = {
        { "list",    l_cls_list    },
        { "exists",  l_cls_exists  },
        { "fields",  l_cls_fields  },
        { "methods", l_cls_methods },
        { "get",     l_cls_get     },
        { "set",     l_cls_set     },
        { "call",    l_cls_call    },
        { nullptr,   nullptr       },
    };

    lua_newtable(L);
    for (const luaL_Reg* f = kCls; f->name; ++f) {
        lua_pushcfunction(L, f->func);
        lua_setfield(L, -2, f->name);
    }
    lua_setglobal(L, "cls");
}

}
