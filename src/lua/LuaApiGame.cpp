
#include "LuaEngine.hpp"

#include <Geode/Geode.hpp>
#include <Geode/binding/FMODAudioEngine.hpp>
#include <Geode/binding/GameObject.hpp>
#include <Geode/binding/GJBaseGameLayer.hpp>
#include <Geode/binding/PlayLayer.hpp>
#include <Geode/binding/PlayerObject.hpp>

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"

extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}

#include <cmath>
#include <concepts>
#include <cstring>
#include <string>
#include <vector>

using namespace geode::prelude;

namespace nh::lua {
namespace {

int l_gd_frame_time(lua_State* L) {
    lua_pushnumber(L, ImGui::GetIO().DeltaTime);
    return 1;
}

int l_gd_fps(lua_State* L) {
    lua_pushnumber(L, ImGui::GetIO().Framerate);
    return 1;
}

char kObjectMetaKey = 0;

constexpr int kMaxHandles = 8192;
constexpr int kMaxScan    = 4096;

std::vector<GameObject*> g_objects;
PlayLayer*               g_registryLayer = nullptr;

void syncRegistry() {
    PlayLayer* pl = PlayLayer::get();
    if (pl != g_registryLayer) {
        g_registryLayer = pl;
        g_objects.clear();
    }
}

int storeObject(lua_State* L, GameObject* obj) {
    syncRegistry();

    if ((int)g_objects.size() >= kMaxHandles)
        return luaL_error(L, "too many object handles are alive at once");

    g_objects.push_back(obj);
    return (int)g_objects.size();
}

GameObject* objectFromHandle(int handle) {
    if (PlayLayer::get() != g_registryLayer) return nullptr;
    if (handle < 1 || handle > (int)g_objects.size()) return nullptr;
    return g_objects[handle - 1];
}

void pushObjectHandle(lua_State* L, GameObject* obj) {
    const int handle = storeObject(L, obj);

    lua_newtable(L);
    lua_pushinteger(L, handle);
    lua_setfield(L, -2, "__handle");

    lua_pushlightuserdata(L, &kObjectMetaKey);
    lua_rawget(L, LUA_REGISTRYINDEX);
    lua_setmetatable(L, -2);
}

GameObject* objectArg(lua_State* L) {
    luaL_checktype(L, 1, LUA_TTABLE);

    lua_getfield(L, 1, "__handle");
    const int handle = (int)lua_tointeger(L, -1);
    lua_pop(L, 1);

    GameObject* obj = objectFromHandle(handle);
    if (!obj)
        luaL_error(L, "this object handle is no longer valid (the level changed)");

    return obj;
}

template <class T>
concept GroupsArrayPtr = requires(T* o) {
    { (*o->m_groups)[0] } -> std::convertible_to<int>;
    o->m_groupCount;
};

template <class T>
concept GroupsRawPtr = requires(T* o) {
    { o->m_groups[0] } -> std::convertible_to<int>;
    o->m_groupCount;
};

template <class T>
concept HasDetailSprite = requires(T* o) { o->m_detailSprite; };

template <class T>
concept HasToggleGroup4 = requires(T* l) { l->toggleGroupTriggered(1, true, nullptr, 0); };

template <class T>
concept HasToggleGroup3 = requires(T* l) { l->toggleGroupTriggered(1, true, nullptr); };

template <class T>
concept HasToggleGroup2 = requires(T* l) { l->toggleGroupTriggered(1, true); };

template <class T>
concept HasMini = requires(T* p) { p->m_vehicleSize; };

template <class T>
concept HasGravityMod = requires(T* p) { p->m_gravityMod; };

template <class T>
concept HasMusicTime = requires(T* e) { { e->getMusicTimeMS(0u) } -> std::convertible_to<double>; };

template <class Obj>
int groupCountOfImpl(Obj* o) {
    if constexpr (GroupsArrayPtr<Obj> || GroupsRawPtr<Obj>) {
        if (!o->m_groups) return 0;
        return (int)o->m_groupCount;
    }
    else {
        (void)o;
        return 0;
    }
}

template <class Obj>
int groupAtImpl(Obj* o, int index) {
    if constexpr (GroupsArrayPtr<Obj>) {
        return (int)(*o->m_groups)[index];
    }
    else if constexpr (GroupsRawPtr<Obj>) {
        return (int)o->m_groups[index];
    }
    else {
        (void)o; (void)index;
        return 0;
    }
}

template <class Obj, class Fn>
void withDetailSprite(Obj* o, Fn&& fn) {
    if constexpr (HasDetailSprite<Obj>) {
        if (o->m_detailSprite) fn(o->m_detailSprite);
    }
    else {
        (void)o; (void)fn;
    }
}

template <class Layer>
bool toggleGroupImpl(Layer* layer, int group, bool on) {
    if constexpr (HasToggleGroup4<Layer>) {
        layer->toggleGroupTriggered(group, on, nullptr, 0);
        return true;
    }
    else if constexpr (HasToggleGroup3<Layer>) {
        layer->toggleGroupTriggered(group, on, nullptr);
        return true;
    }
    else if constexpr (HasToggleGroup2<Layer>) {
        layer->toggleGroupTriggered(group, on);
        return true;
    }
    else {
        (void)layer; (void)group; (void)on;
        return false;
    }
}

int groupCountOf(GameObject* o) {
    return groupCountOfImpl(o);
}

int groupAt(GameObject* o, int index) {
    return groupAtImpl(o, index);
}

bool objectInGroup(GameObject* o, int group) {
    const int count = groupCountOf(o);
    for (int i = 0; i < count; ++i)
        if (groupAt(o, i) == group) return true;
    return false;
}

int l_obj_position(lua_State* L) {
    GameObject* o = objectArg(L);
    lua_pushnumber(L, o->getPositionX());
    lua_pushnumber(L, o->getPositionY());
    return 2;
}

int l_obj_set_position(lua_State* L) {
    GameObject* o = objectArg(L);
    o->setPosition(ccp((float)luaL_checknumber(L, 2), (float)luaL_checknumber(L, 3)));
    return 0;
}

int l_obj_move(lua_State* L) {
    GameObject* o = objectArg(L);
    const float dx = (float)luaL_checknumber(L, 2);
    const float dy = (float)luaL_checknumber(L, 3);
    o->setPosition(ccp(o->getPositionX() + dx, o->getPositionY() + dy));
    return 0;
}

int l_obj_rotation(lua_State* L) {
    lua_pushnumber(L, objectArg(L)->getRotation());
    return 1;
}

int l_obj_set_rotation(lua_State* L) {
    GameObject* o = objectArg(L);
    o->setRotation((float)luaL_checknumber(L, 2));
    return 0;
}

int l_obj_scale(lua_State* L) {
    GameObject* o = objectArg(L);
    lua_pushnumber(L, o->getScaleX());
    lua_pushnumber(L, o->getScaleY());
    return 2;
}

int l_obj_set_scale(lua_State* L) {
    GameObject* o = objectArg(L);
    const float sx = (float)luaL_checknumber(L, 2);
    o->setScaleX(sx);
    o->setScaleY((float)luaL_optnumber(L, 3, sx));
    return 0;
}

int l_obj_color(lua_State* L) {
    GameObject* o = objectArg(L);
    const ccColor3B c = o->getColor();
    lua_pushnumber(L, c.r);
    lua_pushnumber(L, c.g);
    lua_pushnumber(L, c.b);
    return 3;
}

int l_obj_set_color(lua_State* L) {
    GameObject* o = objectArg(L);

    const auto clamp255 = [](double v) -> GLubyte {
        if (v < 0.0)   v = 0.0;
        if (v > 255.0) v = 255.0;
        return (GLubyte)v;
    };

    const ccColor3B c = {
        clamp255(luaL_checknumber(L, 2)),
        clamp255(luaL_checknumber(L, 3)),
        clamp255(luaL_checknumber(L, 4)),
    };

    o->setColor(c);

    withDetailSprite(o, [&](auto* sprite) { sprite->setColor(c); });
    return 0;
}

int l_obj_opacity(lua_State* L) {
    lua_pushnumber(L, objectArg(L)->getOpacity());
    return 1;
}

int l_obj_set_opacity(lua_State* L) {
    GameObject* o = objectArg(L);

    double v = luaL_checknumber(L, 2);
    if (v < 0.0)   v = 0.0;
    if (v > 255.0) v = 255.0;

    o->setOpacity((GLubyte)v);

    withDetailSprite(o, [&](auto* sprite) { sprite->setOpacity((GLubyte)v); });
    return 0;
}

int l_obj_visible(lua_State* L) {
    lua_pushboolean(L, objectArg(L)->isVisible());
    return 1;
}

int l_obj_set_visible(lua_State* L) {
    GameObject* o = objectArg(L);
    o->setVisible(lua_toboolean(L, 2) != 0);
    return 0;
}

int l_obj_id(lua_State* L) {
    lua_pushinteger(L, objectArg(L)->m_objectID);
    return 1;
}

int l_obj_type(lua_State* L) {
    lua_pushinteger(L, (int)objectArg(L)->m_objectType);
    return 1;
}

int l_obj_groups(lua_State* L) {
    GameObject* o = objectArg(L);

    const int count = groupCountOf(o);
    lua_createtable(L, count, 0);
    for (int i = 0; i < count; ++i) {
        lua_pushinteger(L, groupAt(o, i));
        lua_rawseti(L, -2, i + 1);
    }
    return 1;
}

int l_obj_in_group(lua_State* L) {
    GameObject* o = objectArg(L);
    lua_pushboolean(L, objectInGroup(o, (int)luaL_checkinteger(L, 2)));
    return 1;
}

int l_obj_is_decoration(lua_State* L) {
    GameObject* o = objectArg(L);
    lua_pushboolean(L, o->m_isDecoration || o->m_isDecoration2);
    return 1;
}

std::vector<GameObject*> collectObjects() {
    std::vector<GameObject*> out;

    PlayLayer* pl = PlayLayer::get();
    if (!pl || !pl->m_objects) return out;

    for (GameObject* o : CCArrayExt<GameObject*>(pl->m_objects)) {
        if (!o) continue;
        out.push_back(o);
        if ((int)out.size() >= kMaxScan) break;
    }
    return out;
}

int l_objects_count(lua_State* L) {
    PlayLayer* pl = PlayLayer::get();
    lua_pushinteger(L, (pl && pl->m_objects) ? pl->m_objects->count() : 0);
    return 1;
}

int pushObjectList(lua_State* L, const std::vector<GameObject*>& list) {
    lua_createtable(L, (int)list.size(), 0);
    for (std::size_t i = 0; i < list.size(); ++i) {
        pushObjectHandle(L, list[i]);
        lua_rawseti(L, -2, (int)i + 1);
    }
    return 1;
}

int l_objects_all(lua_State* L) {
    return pushObjectList(L, collectObjects());
}

int l_objects_by_id(lua_State* L) {
    const int id = (int)luaL_checkinteger(L, 1);

    std::vector<GameObject*> out;
    for (GameObject* o : collectObjects())
        if (o->m_objectID == id) out.push_back(o);

    return pushObjectList(L, out);
}

int l_objects_by_group(lua_State* L) {
    const int group = (int)luaL_checkinteger(L, 1);

    std::vector<GameObject*> out;
    for (GameObject* o : collectObjects())
        if (objectInGroup(o, group)) out.push_back(o);

    return pushObjectList(L, out);
}

int l_objects_near(lua_State* L) {
    const float x = (float)luaL_checknumber(L, 1);
    const float y = (float)luaL_checknumber(L, 2);
    const float r = (float)luaL_checknumber(L, 3);

    const float r2 = r * r;

    std::vector<GameObject*> out;
    for (GameObject* o : collectObjects()) {
        const float dx = o->getPositionX() - x;
        const float dy = o->getPositionY() - y;
        if (dx * dx + dy * dy <= r2) out.push_back(o);
    }

    return pushObjectList(L, out);
}

int l_objects_toggle_group(lua_State* L) {
    const int  group = (int)luaL_checkinteger(L, 1);
    const bool on    = lua_toboolean(L, 2) != 0;

    GJBaseGameLayer* gl = GJBaseGameLayer::get();
    if (!gl) {
        lua_pushboolean(L, 0);
        return 1;
    }

    lua_pushboolean(L, toggleGroupImpl(gl, group, on) ? 1 : 0);
    return 1;
}

PlayerObject* playerOf(lua_State* L, int index) {
    PlayLayer* pl = PlayLayer::get();
    if (!pl) return nullptr;
    return index == 2 ? pl->m_player2 : pl->m_player1;
}

int l_player_state(lua_State* L) {
    PlayerObject* p = playerOf(L, (int)luaL_optinteger(L, 1, 1));
    if (!p) { lua_pushnil(L); return 1; }

    lua_newtable(L);

    const auto field = [&](const char* key, double value) {
        lua_pushnumber(L, value);
        lua_setfield(L, -2, key);
    };
    const auto flag = [&](const char* key, bool value) {
        lua_pushboolean(L, value);
        lua_setfield(L, -2, key);
    };

    field("x",          p->getPositionX());
    field("y",          p->getPositionY());
    field("rotation",   p->getRotation());
    field("y_velocity", p->m_yVelocity);
    field("scale",      p->getScale());
    flag ("dead",       p->m_isDead);
    flag ("on_ground",  p->m_isOnGround);
    flag ("upside_down", p->m_isUpsideDown);

    if constexpr (HasMini<PlayerObject>)
        flag("mini", p->m_vehicleSize < 1.f);

    if constexpr (HasGravityMod<PlayerObject>)
        field("gravity", p->m_gravityMod);

    return 1;
}

int l_player_rotation(lua_State* L) {
    PlayerObject* p = playerOf(L, (int)luaL_optinteger(L, 1, 1));
    lua_pushnumber(L, p ? p->getRotation() : 0.0);
    return 1;
}

int l_player_pos(lua_State* L) {
    PlayerObject* p = playerOf(L, (int)luaL_optinteger(L, 1, 1));
    if (!p) { lua_pushnil(L); return 1; }
    lua_pushnumber(L, p->getPositionX());
    lua_pushnumber(L, p->getPositionY());
    return 2;
}

int l_player_velocity(lua_State* L) {
    PlayerObject* p = playerOf(L, (int)luaL_optinteger(L, 1, 1));
    lua_pushnumber(L, p ? p->m_yVelocity : 0.0);
    return 1;
}

int l_player_size(lua_State* L) {
    PlayerObject* p = playerOf(L, (int)luaL_optinteger(L, 1, 1));
    if (!p) { lua_pushnil(L); return 1; }
    const CCRect rect = p->getObjectRect();
    lua_pushnumber(L, rect.size.width);
    lua_pushnumber(L, rect.size.height);
    return 2;
}

int l_is_dual(lua_State* L) {
    GJBaseGameLayer* gl = GJBaseGameLayer::get();
    lua_pushboolean(L, gl && gl->m_gameState.m_isDualMode ? 1 : 0);
    return 1;
}

int l_set_player_velocity(lua_State* L) {
    PlayerObject* p = playerOf(L, (int)luaL_optinteger(L, 2, 1));
    if (!p) return 0;

    p->m_yVelocity = luaL_checknumber(L, 1);
    return 0;
}

int l_set_player_rotation(lua_State* L) {
    PlayerObject* p = playerOf(L, (int)luaL_optinteger(L, 2, 1));
    if (!p) return 0;

    p->setRotation((float)luaL_checknumber(L, 1));
    return 0;
}

int l_set_player_scale(lua_State* L) {
    PlayerObject* p = playerOf(L, (int)luaL_optinteger(L, 2, 1));
    if (!p) return 0;

    double s = luaL_checknumber(L, 1);
    if (s < 0.05) s = 0.05;
    if (s > 10.0) s = 10.0;

    p->setScale((float)s);
    return 0;
}

int l_press(lua_State* L) {
    GJBaseGameLayer* gl = GJBaseGameLayer::get();
    if (!gl) return 0;

    const bool down    = lua_toboolean(L, 1) != 0;
    const int  button  = (int)luaL_optinteger(L, 2, (int)PlayerButton::Jump);
    const bool player1 = !lua_toboolean(L, 3);

    gl->handleButton(down, button, player1);
    return 0;
}

CCNode* objectLayer() {
    PlayLayer* pl = PlayLayer::get();
    return pl ? pl->m_objectLayer : nullptr;
}

int l_camera(lua_State* L) {
    CCNode* layer = objectLayer();
    if (!layer) { lua_pushnil(L); return 1; }

    lua_pushnumber(L, -layer->getPositionX());
    lua_pushnumber(L, -layer->getPositionY());
    lua_pushnumber(L, layer->getScale());
    return 3;
}

int l_set_camera(lua_State* L) {
    CCNode* layer = objectLayer();
    if (!layer) return 0;

    layer->setPosition(ccp(-(float)luaL_checknumber(L, 1),
                          -(float)luaL_checknumber(L, 2)));
    return 0;
}

int l_set_zoom(lua_State* L) {
    CCNode* layer = objectLayer();
    if (!layer) return 0;

    double z = luaL_checknumber(L, 1);
    if (z < 0.1) z = 0.1;
    if (z > 5.0) z = 5.0;

    layer->setScale((float)z);
    return 0;
}

int l_world_to_screen(lua_State* L) {
    CCNode* layer = objectLayer();
    if (!layer) { lua_pushnil(L); return 1; }

    const CCPoint world = layer->convertToWorldSpace(
        ccp((float)luaL_checknumber(L, 1), (float)luaL_checknumber(L, 2)));

    const CCSize win  = CCDirector::sharedDirector()->getWinSize();
    const ImVec2 disp = ImGui::GetIO().DisplaySize;

    lua_pushnumber(L, world.x / win.width  * disp.x);
    lua_pushnumber(L, (1.f - world.y / win.height) * disp.y);
    return 2;
}

int l_screen_to_world(lua_State* L) {
    CCNode* layer = objectLayer();
    if (!layer) { lua_pushnil(L); return 1; }

    const CCSize win  = CCDirector::sharedDirector()->getWinSize();
    const ImVec2 disp = ImGui::GetIO().DisplaySize;

    if (disp.x <= 0.f || disp.y <= 0.f) { lua_pushnil(L); return 1; }

    const float wx = (float)luaL_checknumber(L, 1) / disp.x * win.width;
    const float wy = (1.f - (float)luaL_checknumber(L, 2) / disp.y) * win.height;

    const CCPoint local = layer->convertToNodeSpace(ccp(wx, wy));

    lua_pushnumber(L, local.x);
    lua_pushnumber(L, local.y);
    return 2;
}

int l_song_time(lua_State* L) {
    if constexpr (HasMusicTime<FMODAudioEngine>) {
        FMODAudioEngine* engine = FMODAudioEngine::sharedEngine();
        if (engine) {
            lua_pushnumber(L, (double)engine->getMusicTimeMS(0u) / 1000.0);
            return 1;
        }
    }

    PlayLayer* pl = PlayLayer::get();
    lua_pushnumber(L, pl ? pl->m_gameState.m_levelTime : 0.0);
    return 1;
}

int l_frame(lua_State* L) {
    PlayLayer* pl = PlayLayer::get();
    lua_pushinteger(L, pl ? (int)pl->m_gameState.m_currentProgress : 0);
    return 1;
}

int l_is_paused(lua_State* L) {
    PlayLayer* pl = PlayLayer::get();
    lua_pushboolean(L, pl ? pl->m_isPaused : false);
    return 1;
}

void addFunctions(lua_State* L, const char* global, const luaL_Reg* fns) {
    lua_getglobal(L, global);
    if (!lua_istable(L, -1)) { lua_pop(L, 1); return; }

    for (const luaL_Reg* f = fns; f->name; ++f) {
        lua_pushcfunction(L, f->func);
        lua_setfield(L, -2, f->name);
    }
    lua_pop(L, 1);
}

}

void registerGameApi(lua_State* L) {

    static const luaL_Reg kObjectMethods[] = {
        { "position",      l_obj_position      },
        { "set_position",  l_obj_set_position  },
        { "move",          l_obj_move          },
        { "rotation",      l_obj_rotation      },
        { "set_rotation",  l_obj_set_rotation  },
        { "scale",         l_obj_scale         },
        { "set_scale",     l_obj_set_scale     },
        { "color",         l_obj_color         },
        { "set_color",     l_obj_set_color     },
        { "opacity",       l_obj_opacity       },
        { "set_opacity",   l_obj_set_opacity   },
        { "visible",       l_obj_visible       },
        { "set_visible",   l_obj_set_visible   },
        { "id",            l_obj_id            },
        { "type",          l_obj_type          },
        { "groups",        l_obj_groups        },
        { "in_group",      l_obj_in_group      },
        { "is_decoration", l_obj_is_decoration },
        { nullptr,         nullptr             },
    };

    lua_pushlightuserdata(L, &kObjectMetaKey);
    lua_newtable(L);
    lua_newtable(L);
    for (const luaL_Reg* f = kObjectMethods; f->name; ++f) {
        lua_pushcfunction(L, f->func);
        lua_setfield(L, -2, f->name);
    }
    lua_setfield(L, -2, "__index");
    lua_pushboolean(L, 0);
    lua_setfield(L, -2, "__metatable");
    lua_rawset(L, LUA_REGISTRYINDEX);

    static const luaL_Reg kObjects[] = {
        { "count",        l_objects_count        },
        { "all",          l_objects_all          },
        { "by_id",        l_objects_by_id        },
        { "by_group",     l_objects_by_group     },
        { "near",         l_objects_near         },
        { "toggle_group", l_objects_toggle_group },
        { nullptr,        nullptr                },
    };

    lua_getglobal(L, "gd");
    if (lua_istable(L, -1)) {
        lua_newtable(L);
        for (const luaL_Reg* f = kObjects; f->name; ++f) {
            lua_pushcfunction(L, f->func);
            lua_setfield(L, -2, f->name);
        }
        lua_setfield(L, -2, "objects");
    }
    lua_pop(L, 1);

    static const luaL_Reg kGameExtras[] = {
        { "player_state",        l_player_state        },
        { "player_rotation",     l_player_rotation     },
        { "player_pos",          l_player_pos          },
        { "player_velocity",     l_player_velocity     },
        { "player_size",         l_player_size         },
        { "is_dual",             l_is_dual             },
        { "set_player_velocity", l_set_player_velocity },
        { "set_player_rotation", l_set_player_rotation },
        { "set_player_scale",    l_set_player_scale    },
        { "press",               l_press               },
        { "camera",              l_camera              },
        { "set_camera",          l_set_camera          },
        { "set_zoom",            l_set_zoom            },
        { "world_to_screen",     l_world_to_screen     },
        { "screen_to_world",     l_screen_to_world     },
        { "song_time",           l_song_time           },
        { "frame",               l_frame               },
        { "frame_time",          l_gd_frame_time       },
        { "fps",                 l_gd_fps              },
        { "is_paused",           l_is_paused           },
        { nullptr,               nullptr               },
    };
    addFunctions(L, "gd", kGameExtras);
}

}
