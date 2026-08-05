
#include "LuaEngine.hpp"
#include "LuaHooks.hpp"

extern "C" {
#include <lauxlib.h>
#include <lua.h>
}

namespace nh::lua {

namespace {

constexpr int kMaxPerHook = 32;

void pushHooksTable(lua_State* L) {
    lua_pushlightuserdata(L, hooksRegistryKey());
    lua_rawget(L, LUA_REGISTRYINDEX);
}

void pushHookList(lua_State* L, const char* name) {
    pushHooksTable(L);
    lua_getfield(L, -1, name);

    if (!lua_istable(L, -1)) {
        lua_pop(L, 1);
        lua_newtable(L);
        lua_pushvalue(L, -1);
        lua_setfield(L, -3, name);
    }
}

HookId hookArg(lua_State* L, int index) {
    const char* name = luaL_checkstring(L, index);

    HookId id;
    if (!hookFromName(name, id))
        luaL_error(L, "unknown hook '%s' (see hooks.list())", name);

    return id;
}

int l_hooks_add(lua_State* L) {
    const HookId id = hookArg(L, 1);
    luaL_checktype(L, 2, LUA_TFUNCTION);

    pushHookList(L, hookDef(id).name);

    const int index = (int)lua_objlen(L, -1) + 1;
    if (index > kMaxPerHook) {
        lua_pop(L, 2);
        return luaL_error(L, "too many callbacks on '%s' (limit %d)",
                          hookDef(id).name, kMaxPerHook);
    }

    lua_pushvalue(L, 2);
    lua_rawseti(L, -2, index);
    lua_pop(L, 2);

    markHookListenersDirty();

    lua_pushinteger(L, index);
    return 1;
}

int l_hooks_remove(lua_State* L) {
    const HookId id    = hookArg(L, 1);
    const int    index = (int)luaL_checkinteger(L, 2);

    pushHookList(L, hookDef(id).name);

    if (index >= 1 && index <= (int)lua_objlen(L, -1)) {
        lua_pushboolean(L, 0);
        lua_rawseti(L, -2, index);
    }

    lua_pop(L, 2);
    markHookListenersDirty();
    return 0;
}

int l_hooks_clear(lua_State* L) {
    if (lua_isnoneornil(L, 1)) {
        lua_pushlightuserdata(L, hooksRegistryKey());
        lua_newtable(L);
        lua_rawset(L, LUA_REGISTRYINDEX);

        markHookListenersDirty();
        return 0;
    }

    const HookId id = hookArg(L, 1);

    pushHooksTable(L);
    lua_newtable(L);
    lua_setfield(L, -2, hookDef(id).name);
    lua_pop(L, 1);

    markHookListenersDirty();
    return 0;
}

int l_hooks_list(lua_State* L) {
    lua_newtable(L);

    for (int i = 0; i < hookCount(); ++i) {
        const HookDef& def = hookDef((HookId)i);

        lua_newtable(L);

        lua_pushstring(L, def.name);
        lua_setfield(L, -2, "name");

        lua_pushboolean(L, def.cancelable ? 1 : 0);
        lua_setfield(L, -2, "cancelable");

        pushHooksTable(L);
        lua_getfield(L, -1, def.name);
        const int count = lua_istable(L, -1) ? (int)lua_objlen(L, -1) : 0;
        lua_pop(L, 2);

        lua_pushinteger(L, count);
        lua_setfield(L, -2, "count");

        lua_rawseti(L, -2, i + 1);
    }

    return 1;
}

}

void registerHooksApi(lua_State* L) {

    lua_pushlightuserdata(L, hooksRegistryKey());
    lua_newtable(L);
    lua_rawset(L, LUA_REGISTRYINDEX);

    static const luaL_Reg kHooks[] = {
        { "add",    l_hooks_add    },
        { "remove", l_hooks_remove },
        { "clear",  l_hooks_clear  },
        { "list",   l_hooks_list   },
        { nullptr,  nullptr        },
    };

    lua_newtable(L);
    for (const luaL_Reg* f = kHooks; f->name; ++f) {
        lua_pushcfunction(L, f->func);
        lua_setfield(L, -2, f->name);
    }
    lua_setglobal(L, "hooks");

    markHookListenersDirty();
}

}
