
#include "LuaEngine.hpp"

#include <Geode/Geode.hpp>
#include <Geode/binding/GJGameLevel.hpp>
#include <Geode/binding/PlayLayer.hpp>
#include <Geode/binding/PlayerObject.hpp>

#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include "imgui.h"

#include "../gui/binds.h"

extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}

#include <cstring>
#include <string>

using namespace geode::prelude;

namespace nh::lua {

namespace {

char kTabMetaKey = 0;

Script* self(lua_State* L) {
    Script* s = ownerOf(L);
    if (!s) luaL_error(L, "script context lost");
    return s;
}

ImU32 colorArg(lua_State* L, int first) {
    const float r = (float)luaL_optnumber(L, first + 0, 255.0) / 255.f;
    const float g = (float)luaL_optnumber(L, first + 1, 255.0) / 255.f;
    const float b = (float)luaL_optnumber(L, first + 2, 255.0) / 255.f;
    const float a = (float)luaL_optnumber(L, first + 3, 255.0) / 255.f;
    return ImGui::GetColorU32(ImVec4(r, g, b, a));
}

MenuTab* tabArg(lua_State* L) {
    luaL_checktype(L, 1, LUA_TTABLE);
    lua_getfield(L, 1, "__tab");
    const int index = (int)lua_tointeger(L, -1);
    lua_pop(L, 1);

    MenuTab* t = Manager::get().tabAt(index);
    if (!t) luaL_error(L, "this tab no longer exists");
    return t;
}

MenuWidget* findWidget(MenuTab* tab, const char* id) {
    for (auto& w : tab->widgets)
        if (w.id == id) return &w;
    return nullptr;
}

void pushRegistryTable(lua_State* L, void* key) {
    lua_pushlightuserdata(L, key);
    lua_rawget(L, LUA_REGISTRYINDEX);
}

float colorComponent(double raw) {
    const double v = raw / 255.0;
    return (float)(v < 0.0 ? 0.0 : (v > 1.0 ? 1.0 : v));
}

bool readColorField(lua_State* L, int table, const char* field, float out[4]) {
    lua_getfield(L, table, field);
    if (!lua_istable(L, -1)) { lua_pop(L, 1); return false; }

    const int count = (int)lua_objlen(L, -1);
    for (int i = 0; i < 4; ++i) {
        if (i < count) {
            lua_rawgeti(L, -1, i + 1);
            out[i] = colorComponent(lua_tonumber(L, -1));
            lua_pop(L, 1);
        }
        else if (i == 3) {
            out[3] = 1.f;
        }
    }

    lua_pop(L, 1);
    return true;
}

void readStyle(lua_State* L, int index, TextStyle& style) {
    if (!lua_istable(L, index)) return;

    if (readColorField(L, index, "color", style.colorA)) style.colored = true;

    if (readColorField(L, index, "gradient", style.colorB)) {
        style.gradient = true;
        style.colored  = true;
    }

    lua_getfield(L, index, "animated");
    if (lua_toboolean(L, -1)) style.animated = true;
    lua_pop(L, 1);

    lua_getfield(L, index, "rainbow");
    if (lua_toboolean(L, -1)) {
        style.rainbow = true;
        style.colored = true;
    }
    lua_pop(L, 1);

    lua_getfield(L, index, "speed");
    if (lua_isnumber(L, -1)) {
        const double s = lua_tonumber(L, -1);
        style.speed = (float)(s < 0.05 ? 0.05 : (s > 20.0 ? 20.0 : s));
    }
    lua_pop(L, 1);
}

int l_print(lua_State* L) {
    const int n = lua_gettop(L);
    std::string line;

    for (int i = 1; i <= n; ++i) {
        if (i > 1) line += "\t";

        if (lua_isstring(L, i)) {
            line += lua_tostring(L, i);
        }
        else if (lua_isnil(L, i))     line += "nil";
        else if (lua_isboolean(L, i)) line += lua_toboolean(L, i) ? "true" : "false";
        else                          line += luaL_typename(L, i);
    }

    Manager::get().log(self(L)->fileName() + ": " + line);
    return 0;
}

int l_client_log(lua_State* L) {
    Manager::get().log(self(L)->fileName() + ": " + luaL_checkstring(L, 1));
    return 0;
}

int l_client_error(lua_State* L) {
    Manager::get().log(self(L)->fileName() + ": " + luaL_checkstring(L, 1), true);
    return 0;
}

int l_client_frame_time(lua_State* L) {
    lua_pushnumber(L, ImGui::GetIO().DeltaTime);
    return 1;
}

int l_client_fps(lua_State* L) {
    lua_pushnumber(L, ImGui::GetIO().Framerate);
    return 1;
}

int l_client_key_down(lua_State* L) {
    lua_pushboolean(L, nh_vk_down((int)luaL_checkinteger(L, 1)));
    return 1;
}

int l_client_menu_open(lua_State* L) {
    lua_pushboolean(L, Vars::menuOpen);
    return 1;
}

int l_callbacks_add(lua_State* L) {
    const char* name = luaL_checkstring(L, 1);
    luaL_checktype(L, 2, LUA_TFUNCTION);

    Event e;
    if (!eventFromName(name, e))
        return luaL_error(L, "unknown event '%s'", name);

    pushRegistryTable(L, callbacksRegistryKey());
    lua_getfield(L, -1, eventName(e));

    const int index = (int)lua_objlen(L, -1) + 1;
    lua_pushvalue(L, 2);
    lua_rawseti(L, -2, index);

    lua_pop(L, 2);

    lua_pushinteger(L, index);
    return 1;
}

int l_callbacks_remove(lua_State* L) {
    const char* name  = luaL_checkstring(L, 1);
    const int   index = (int)luaL_checkinteger(L, 2);

    Event e;
    if (!eventFromName(name, e))
        return luaL_error(L, "unknown event '%s'", name);

    pushRegistryTable(L, callbacksRegistryKey());
    lua_getfield(L, -1, eventName(e));

    if (lua_istable(L, -1) && index >= 1 && index <= (int)lua_objlen(L, -1)) {
        lua_pushboolean(L, 0);
        lua_rawseti(L, -2, index);
    }

    lua_pop(L, 2);
    return 0;
}

int l_callbacks_clear(lua_State* L) {
    const char* name = luaL_checkstring(L, 1);

    Event e;
    if (!eventFromName(name, e))
        return luaL_error(L, "unknown event '%s'", name);

    pushRegistryTable(L, callbacksRegistryKey());
    lua_newtable(L);
    lua_setfield(L, -2, eventName(e));
    lua_pop(L, 1);
    return 0;
}

int l_tab_toggle(lua_State* L) {
    MenuTab* t = tabArg(L);

    MenuWidget w;
    w.type   = WidgetType::Toggle;
    w.id     = luaL_checkstring(L, 2);
    w.label  = luaL_checkstring(L, 3);
    w.bValue = lua_toboolean(L, 4) != 0;

    if (findWidget(t, w.id.c_str()))
        return luaL_error(L, "widget id '%s' is already used in this tab", w.id.c_str());

    t->widgets.push_back(std::move(w));
    lua_pushvalue(L, 1);
    return 1;
}

int l_tab_slider(lua_State* L) {
    MenuTab* t = tabArg(L);

    MenuWidget w;
    w.type   = WidgetType::Slider;
    w.id     = luaL_checkstring(L, 2);
    w.label  = luaL_checkstring(L, 3);
    w.minVal = (float)luaL_checknumber(L, 4);
    w.maxVal = (float)luaL_checknumber(L, 5);
    w.fValue = (float)luaL_optnumber(L, 6, w.minVal);

    if (findWidget(t, w.id.c_str()))
        return luaL_error(L, "widget id '%s' is already used in this tab", w.id.c_str());

    t->widgets.push_back(std::move(w));
    lua_pushvalue(L, 1);
    return 1;
}

int l_tab_slider_int(lua_State* L) {
    MenuTab* t = tabArg(L);

    MenuWidget w;
    w.type   = WidgetType::SliderInt;
    w.id     = luaL_checkstring(L, 2);
    w.label  = luaL_checkstring(L, 3);
    w.minVal = (float)luaL_checkinteger(L, 4);
    w.maxVal = (float)luaL_checkinteger(L, 5);
    w.iValue = (int)luaL_optinteger(L, 6, (int)w.minVal);

    if (findWidget(t, w.id.c_str()))
        return luaL_error(L, "widget id '%s' is already used in this tab", w.id.c_str());

    t->widgets.push_back(std::move(w));
    lua_pushvalue(L, 1);
    return 1;
}

int l_tab_combo(lua_State* L) {
    MenuTab* t = tabArg(L);

    MenuWidget w;
    w.type  = WidgetType::Combo;
    w.id    = luaL_checkstring(L, 2);
    w.label = luaL_checkstring(L, 3);
    luaL_checktype(L, 4, LUA_TTABLE);

    const int count = (int)lua_objlen(L, 4);
    for (int i = 1; i <= count; ++i) {
        lua_rawgeti(L, 4, i);
        w.items.push_back(lua_isstring(L, -1) ? lua_tostring(L, -1) : "?");
        lua_pop(L, 1);
    }
    if (w.items.empty())
        return luaL_error(L, "combo '%s' needs at least one item", w.id.c_str());

    w.iValue = (int)luaL_optinteger(L, 5, 1) - 1;
    if (w.iValue < 0 || w.iValue >= (int)w.items.size()) w.iValue = 0;

    if (findWidget(t, w.id.c_str()))
        return luaL_error(L, "widget id '%s' is already used in this tab", w.id.c_str());

    t->widgets.push_back(std::move(w));
    lua_pushvalue(L, 1);
    return 1;
}

int l_tab_button(lua_State* L) {
    MenuTab* t = tabArg(L);

    MenuWidget w;
    w.type  = WidgetType::Button;
    w.label = luaL_checkstring(L, 2);
    luaL_checktype(L, 3, LUA_TFUNCTION);

    lua_pushvalue(L, 3);
    w.fnRef = luaL_ref(L, LUA_REGISTRYINDEX);

    t->widgets.push_back(std::move(w));
    lua_pushvalue(L, 1);
    return 1;
}

int l_tab_label(lua_State* L) {
    MenuTab* t = tabArg(L);

    MenuWidget w;
    w.type  = WidgetType::Label;
    w.label = luaL_checkstring(L, 2);

    if (lua_istable(L, 3)) readStyle(L, 3, w.style);

    t->widgets.push_back(std::move(w));
    lua_pushvalue(L, 1);
    return 1;
}

int l_tab_separator(lua_State* L) {
    MenuTab* t = tabArg(L);

    MenuWidget w;
    w.type = WidgetType::Separator;

    t->widgets.push_back(std::move(w));
    lua_pushvalue(L, 1);
    return 1;
}

int l_tab_input_text(lua_State* L) {
    MenuTab* t = tabArg(L);

    MenuWidget w;
    w.type   = WidgetType::InputText;
    w.id     = luaL_checkstring(L, 2);
    w.label  = luaL_checkstring(L, 3);
    w.sValue = luaL_optstring(L, 4, "");

    if (w.sValue.size() > 480) w.sValue.resize(480);

    if (findWidget(t, w.id.c_str()))
        return luaL_error(L, "widget id '%s' is already used in this tab", w.id.c_str());

    t->widgets.push_back(std::move(w));
    lua_pushvalue(L, 1);
    return 1;
}

int l_tab_color_picker(lua_State* L) {
    MenuTab* t = tabArg(L);

    MenuWidget w;
    w.type  = WidgetType::ColorPicker;
    w.id    = luaL_checkstring(L, 2);
    w.label = luaL_checkstring(L, 3);

    for (int i = 0; i < 4; ++i)
        w.color[i] = colorComponent(luaL_optnumber(L, 4 + i, 255.0));

    if (findWidget(t, w.id.c_str()))
        return luaL_error(L, "widget id '%s' is already used in this tab", w.id.c_str());

    t->widgets.push_back(std::move(w));
    lua_pushvalue(L, 1);
    return 1;
}

int l_tab_group(lua_State* L) {
    MenuTab* t = tabArg(L);

    MenuWidget w;
    w.type  = WidgetType::GroupBegin;
    w.label = luaL_checkstring(L, 2);

    t->widgets.push_back(std::move(w));
    lua_pushvalue(L, 1);
    return 1;
}

int l_tab_group_end(lua_State* L) {
    MenuTab* t = tabArg(L);

    MenuWidget w;
    w.type = WidgetType::GroupEnd;

    t->widgets.push_back(std::move(w));
    lua_pushvalue(L, 1);
    return 1;
}

int l_tab_bindable(lua_State* L) {
    MenuTab* t = tabArg(L);

    MenuWidget w;
    w.type  = WidgetType::Toggle;
    w.id    = luaL_checkstring(L, 2);
    w.label = luaL_checkstring(L, 3);

    if (findWidget(t, w.id.c_str()))
        return luaL_error(L, "widget id '%s' is already used in this tab", w.id.c_str());

    const bool initial = lua_toboolean(L, 4) != 0;

    w.bBox     = Manager::get().makeBindBox(initial);
    w.bValue   = initial;
    w.bindName = luaL_optstring(L, 5, w.label.c_str());

    BindSystem::get().category(("Scripts/" + t->title).c_str());
    BindSystem::get().registerBool(w.bindName, w.bBox.get());

    t->widgets.push_back(std::move(w));
    lua_pushvalue(L, 1);
    return 1;
}

int l_tab_on_change(lua_State* L) {
    MenuTab*    t  = tabArg(L);
    const char* id = luaL_checkstring(L, 2);
    luaL_checktype(L, 3, LUA_TFUNCTION);

    MenuWidget* w = findWidget(t, id);
    if (!w) return luaL_error(L, "no widget with id '%s'", id);

    if (w->changedRef >= 0) self(L)->freeRef(w->changedRef);

    lua_pushvalue(L, 3);
    w->changedRef = luaL_ref(L, LUA_REGISTRYINDEX);

    lua_pushvalue(L, 1);
    return 1;
}

int l_tab_text_style(lua_State* L) {
    MenuTab*    t  = tabArg(L);
    const char* id = luaL_checkstring(L, 2);
    luaL_checktype(L, 3, LUA_TTABLE);

    MenuWidget* w = findWidget(t, id);
    if (!w) return luaL_error(L, "no widget with id '%s'", id);

    readStyle(L, 3, w->style);

    lua_pushvalue(L, 1);
    return 1;
}

int l_tab_get(lua_State* L) {
    MenuTab*    t = tabArg(L);
    const char* id = luaL_checkstring(L, 2);

    MenuWidget* w = findWidget(t, id);
    if (!w) return luaL_error(L, "no widget with id '%s'", id);

    switch (w->type) {
    case WidgetType::Toggle:    lua_pushboolean(L, w->boolValue()); return 1;
    case WidgetType::Slider:    lua_pushnumber(L, w->fValue);       return 1;
    case WidgetType::SliderInt: lua_pushinteger(L, w->iValue);      return 1;
    case WidgetType::Combo:     lua_pushinteger(L, w->iValue + 1);  return 1;

    case WidgetType::InputText:
        lua_pushlstring(L, w->sValue.data(), w->sValue.size());
        return 1;

    case WidgetType::ColorPicker:
        for (int i = 0; i < 4; ++i) lua_pushnumber(L, w->color[i] * 255.f);
        return 4;

    default:                    lua_pushnil(L);                     return 1;
    }
}

int l_tab_set(lua_State* L) {
    MenuTab*    t  = tabArg(L);
    const char* id = luaL_checkstring(L, 2);

    MenuWidget* w = findWidget(t, id);
    if (!w) return luaL_error(L, "no widget with id '%s'", id);

    switch (w->type) {
    case WidgetType::Toggle:
        w->setBool(lua_toboolean(L, 3) != 0);
        break;

    case WidgetType::InputText: {
        std::size_t length = 0;
        const char* text   = luaL_checklstring(L, 3, &length);
        if (length > 480) length = 480;
        w->sValue.assign(text, length);
        break;
    }

    case WidgetType::ColorPicker:
        for (int i = 0; i < 4; ++i)
            w->color[i] = colorComponent(luaL_optnumber(L, 3 + i, 255.0));
        break;
    case WidgetType::Slider:
        w->fValue = ImClamp((float)luaL_checknumber(L, 3), w->minVal, w->maxVal);
        break;
    case WidgetType::SliderInt:
        w->iValue = (int)ImClamp((float)luaL_checkinteger(L, 3), w->minVal, w->maxVal);
        break;
    case WidgetType::Combo: {
        const int v = (int)luaL_checkinteger(L, 3) - 1;
        if (v >= 0 && v < (int)w->items.size()) w->iValue = v;
        break;
    }
    default:
        return luaL_error(L, "widget '%s' has no value", id);
    }
    return 0;
}

int l_menu_tab(lua_State* L) {
    const char* title = luaL_checkstring(L, 1);
    Script*     s     = self(L);

    const int index = Manager::get().addTab(s->id(), title);

    lua_newtable(L);
    lua_pushinteger(L, index);
    lua_setfield(L, -2, "__tab");

    pushRegistryTable(L, &kTabMetaKey);
    lua_setmetatable(L, -2);
    return 1;
}

int l_mod_list(lua_State* L) {
    const auto names = BindSystem::get().featureNames();

    lua_newtable(L);
    for (std::size_t i = 0; i < names.size(); ++i) {
        lua_pushstring(L, names[i].c_str());
        lua_rawseti(L, -2, (int)i + 1);
    }
    return 1;
}

int l_mod_get(lua_State* L) {
    bool value = false;
    if (!BindSystem::get().getFeatureBool(luaL_checkstring(L, 1), &value))
        return luaL_error(L, "unknown feature '%s'", lua_tostring(L, 1));

    lua_pushboolean(L, value);
    return 1;
}

int l_mod_set(lua_State* L) {
    const char* name = luaL_checkstring(L, 1);
    luaL_checkany(L, 2);

    if (!BindSystem::get().setFeatureBool(name, lua_toboolean(L, 2) != 0))
        return luaL_error(L, "unknown feature '%s'", name);
    return 0;
}

int l_mod_get_value(lua_State* L) {
    float value = 0.f;
    if (!BindSystem::get().getFeatureValue(luaL_checkstring(L, 1), &value))
        return luaL_error(L, "feature '%s' has no value", lua_tostring(L, 1));

    lua_pushnumber(L, value);
    return 1;
}

int l_mod_set_value(lua_State* L) {
    const char* name = luaL_checkstring(L, 1);
    const float v    = (float)luaL_checknumber(L, 2);

    if (!BindSystem::get().setFeatureValue(name, v))
        return luaL_error(L, "feature '%s' has no value", name);
    return 0;
}

int l_gd_in_level(lua_State* L) {
    lua_pushboolean(L, PlayLayer::get() != nullptr);
    return 1;
}

int l_gd_level(lua_State* L) {
    auto* pl = PlayLayer::get();
    if (!pl || !pl->m_level) { lua_pushnil(L); return 1; }

    auto* lvl = pl->m_level;

    lua_newtable(L);

    lua_pushstring(L, lvl->m_levelName.c_str());
    lua_setfield(L, -2, "name");

    lua_pushinteger(L, lvl->m_levelID.value());
    lua_setfield(L, -2, "id");

    lua_pushinteger(L, lvl->m_attempts.value());
    lua_setfield(L, -2, "attempts");

    lua_pushnumber(L, lvl->m_normalPercent.value());
    lua_setfield(L, -2, "best");

    lua_pushnumber(L, pl->getCurrentPercent());
    lua_setfield(L, -2, "percent");

    lua_pushboolean(L, lvl->isPlatformer());
    lua_setfield(L, -2, "platformer");

    lua_pushboolean(L, pl->m_isPracticeMode);
    lua_setfield(L, -2, "practice");

    return 1;
}

PlayerObject* playerArg(lua_State* L, int index) {
    auto* pl = PlayLayer::get();
    if (!pl) return nullptr;

    return luaL_optinteger(L, index, 1) == 2 ? pl->m_player2 : pl->m_player1;
}

int l_gd_player(lua_State* L) {
    auto* p = playerArg(L, 1);
    if (!p) { lua_pushnil(L); return 1; }

    lua_newtable(L);

    lua_pushnumber(L, p->getPositionX());
    lua_setfield(L, -2, "x");

    lua_pushnumber(L, p->getPositionY());
    lua_setfield(L, -2, "y");

    lua_pushnumber(L, p->getRotation());
    lua_setfield(L, -2, "rotation");

    lua_pushnumber(L, p->m_yVelocity);
    lua_setfield(L, -2, "y_velocity");

    lua_pushboolean(L, p->m_isDead);
    lua_setfield(L, -2, "dead");

    lua_pushboolean(L, p->m_isOnGround);
    lua_setfield(L, -2, "on_ground");

    lua_pushboolean(L, p->m_isUpsideDown);
    lua_setfield(L, -2, "upside_down");

    return 1;
}

int l_gd_set_player_pos(lua_State* L) {
    auto* p = playerArg(L, 3);
    if (!p) return 0;

    p->setPosition({ (float)luaL_checknumber(L, 1), (float)luaL_checknumber(L, 2) });
    return 0;
}

int l_gd_time(lua_State* L) {
    auto* pl = PlayLayer::get();
    lua_pushnumber(L, pl ? pl->m_gameState.m_levelTime : 0.0);
    return 1;
}

int l_draw_screen_size(lua_State* L) {
    const ImVec2 s = ImGui::GetIO().DisplaySize;
    lua_pushnumber(L, s.x);
    lua_pushnumber(L, s.y);
    return 2;
}

int l_draw_text(lua_State* L) {
    const float x = (float)luaL_checknumber(L, 1);
    const float y = (float)luaL_checknumber(L, 2);
    const char* t = luaL_checkstring(L, 3);

    ImGui::GetForegroundDrawList()->AddText(ImVec2(x, y), colorArg(L, 4), t);
    return 0;
}

int l_draw_text_size(lua_State* L) {
    const ImVec2 s = ImGui::CalcTextSize(luaL_checkstring(L, 1));
    lua_pushnumber(L, s.x);
    lua_pushnumber(L, s.y);
    return 2;
}

int l_draw_rect(lua_State* L) {
    const float x = (float)luaL_checknumber(L, 1);
    const float y = (float)luaL_checknumber(L, 2);
    const float w = (float)luaL_checknumber(L, 3);
    const float h = (float)luaL_checknumber(L, 4);

    ImGui::GetForegroundDrawList()->AddRect(
        ImVec2(x, y), ImVec2(x + w, y + h), colorArg(L, 5), 0.f, 0, 1.f);
    return 0;
}

int l_draw_rect_filled(lua_State* L) {
    const float x = (float)luaL_checknumber(L, 1);
    const float y = (float)luaL_checknumber(L, 2);
    const float w = (float)luaL_checknumber(L, 3);
    const float h = (float)luaL_checknumber(L, 4);

    ImGui::GetForegroundDrawList()->AddRectFilled(
        ImVec2(x, y), ImVec2(x + w, y + h), colorArg(L, 5),
        (float)luaL_optnumber(L, 9, 0.0));
    return 0;
}

int l_draw_line(lua_State* L) {
    const float x1 = (float)luaL_checknumber(L, 1);
    const float y1 = (float)luaL_checknumber(L, 2);
    const float x2 = (float)luaL_checknumber(L, 3);
    const float y2 = (float)luaL_checknumber(L, 4);

    ImGui::GetForegroundDrawList()->AddLine(
        ImVec2(x1, y1), ImVec2(x2, y2), colorArg(L, 5),
        (float)luaL_optnumber(L, 9, 1.0));
    return 0;
}

int l_draw_circle(lua_State* L) {
    const float x = (float)luaL_checknumber(L, 1);
    const float y = (float)luaL_checknumber(L, 2);
    const float r = (float)luaL_checknumber(L, 3);

    ImGui::GetForegroundDrawList()->AddCircle(ImVec2(x, y), r, colorArg(L, 4), 0,
                                              (float)luaL_optnumber(L, 8, 1.0));
    return 0;
}

int l_draw_text_gradient(lua_State* L) {
    const float x    = (float)luaL_checknumber(L, 1);
    const float y    = (float)luaL_checknumber(L, 2);
    const char* text = luaL_checkstring(L, 3);

    const float ar = colorComponent(luaL_optnumber(L, 4, 255.0));
    const float ag = colorComponent(luaL_optnumber(L, 5, 255.0));
    const float ab = colorComponent(luaL_optnumber(L, 6, 255.0));
    const float br = colorComponent(luaL_optnumber(L, 7, 255.0));
    const float bg = colorComponent(luaL_optnumber(L, 8, 255.0));
    const float bb = colorComponent(luaL_optnumber(L, 9, 255.0));
    const float al = colorComponent(luaL_optnumber(L, 10, 255.0));

    auto*             list  = ImGui::GetForegroundDrawList();
    const std::size_t count = std::strlen(text);

    float cursor = x;
    for (std::size_t i = 0; i < count; ++i) {
        const char glyph[2] = { text[i], '\0' };
        const float t = count > 1 ? (float)i / (float)(count - 1) : 0.f;

        const ImU32 color = ImGui::GetColorU32(ImVec4(
            ar + (br - ar) * t,
            ag + (bg - ag) * t,
            ab + (bb - ab) * t,
            al));

        list->AddText(ImVec2(cursor, y), color, glyph, glyph + 1);
        cursor += ImGui::CalcTextSize(glyph).x;
    }
    return 0;
}

int l_draw_text_rainbow(lua_State* L) {
    const float x    = (float)luaL_checknumber(L, 1);
    const float y    = (float)luaL_checknumber(L, 2);
    const char* text = luaL_checkstring(L, 3);

    const float speed = (float)luaL_optnumber(L, 4, 1.0);
    const float alpha = colorComponent(luaL_optnumber(L, 5, 255.0));

    auto*             list  = ImGui::GetForegroundDrawList();
    const std::size_t count = std::strlen(text);
    const float       time  = (float)ImGui::GetTime() * speed;

    float cursor = x;
    for (std::size_t i = 0; i < count; ++i) {
        const char glyph[2] = { text[i], '\0' };

        float hue = time * 0.2f + (float)i * 0.03f;
        hue = hue - (float)(int)hue;

        const ImU32 color = (ImU32)ImColor::HSV(hue, 0.75f, 1.f, alpha);

        list->AddText(ImVec2(cursor, y), color, glyph, glyph + 1);
        cursor += ImGui::CalcTextSize(glyph).x;
    }
    return 0;
}

std::string storageKey(lua_State* L, const char* key) {
    return "script." + self(L)->id() + "." + key;
}

int l_storage_set(lua_State* L) {
    const std::string key = storageKey(L, luaL_checkstring(L, 1));

    if (lua_isboolean(L, 2))
        Mod::get()->setSavedValue<bool>(key, lua_toboolean(L, 2) != 0);
    else if (lua_isnumber(L, 2))
        Mod::get()->setSavedValue<double>(key, lua_tonumber(L, 2));
    else if (lua_isstring(L, 2))
        Mod::get()->setSavedValue<std::string>(key, lua_tostring(L, 2));
    else
        return luaL_error(L, "storage.set only accepts booleans, numbers and strings");

    return 0;
}

int l_storage_get(lua_State* L) {
    const std::string key = storageKey(L, luaL_checkstring(L, 1));

    if (lua_isboolean(L, 2)) {
        lua_pushboolean(L, Mod::get()->getSavedValue<bool>(key, lua_toboolean(L, 2) != 0));
    }
    else if (lua_isnumber(L, 2)) {
        lua_pushnumber(L, Mod::get()->getSavedValue<double>(key, lua_tonumber(L, 2)));
    }
    else if (lua_isstring(L, 2)) {
        lua_pushstring(L,
            Mod::get()->getSavedValue<std::string>(key, lua_tostring(L, 2)).c_str());
    }
    else {

        lua_pushnil(L);
    }
    return 1;
}

int l_utils_clamp(lua_State* L) {
    const double v  = luaL_checknumber(L, 1);
    const double lo = luaL_checknumber(L, 2);
    const double hi = luaL_checknumber(L, 3);
    lua_pushnumber(L, v < lo ? lo : (v > hi ? hi : v));
    return 1;
}

int l_utils_lerp(lua_State* L) {
    const double a = luaL_checknumber(L, 1);
    const double b = luaL_checknumber(L, 2);
    const double t = luaL_checknumber(L, 3);
    lua_pushnumber(L, a + (b - a) * t);
    return 1;
}

int l_utils_round(lua_State* L) {
    lua_pushnumber(L, (double)(long long)(luaL_checknumber(L, 1) + 0.5));
    return 1;
}

void registerTable(lua_State* L, const char* name, const luaL_Reg* fns) {
    lua_newtable(L);
    for (const luaL_Reg* f = fns; f->name; ++f) {
        lua_pushcfunction(L, f->func);
        lua_setfield(L, -2, f->name);
    }
    lua_setglobal(L, name);
}

}

void registerApi(lua_State* L, Script* owner) {
    (void)owner;

    lua_pushcfunction(L, l_print);
    lua_setglobal(L, "print");

    static const luaL_Reg kClient[] = {
        { "log",        l_client_log        },
        { "error",      l_client_error      },
        { "frame_time", l_client_frame_time },
        { "fps",        l_client_fps        },
        { "key_down",   l_client_key_down   },
        { "menu_open",  l_client_menu_open  },
        { nullptr,      nullptr             },
    };
    registerTable(L, "client", kClient);

    static const luaL_Reg kCallbacks[] = {
        { "add",    l_callbacks_add    },
        { "remove", l_callbacks_remove },
        { "clear",  l_callbacks_clear  },
        { nullptr, nullptr           },
    };
    registerTable(L, "callbacks", kCallbacks);

    static const luaL_Reg kMenu[] = {
        { "tab",   l_menu_tab },
        { nullptr, nullptr    },
    };
    registerTable(L, "menu", kMenu);

    static const luaL_Reg kMod[] = {
        { "list",      l_mod_list      },
        { "get",       l_mod_get       },
        { "set",       l_mod_set       },
        { "get_value", l_mod_get_value },
        { "set_value", l_mod_set_value },
        { nullptr,     nullptr         },
    };
    registerTable(L, "mod", kMod);

    static const luaL_Reg kGd[] = {
        { "in_level",       l_gd_in_level       },
        { "level",          l_gd_level          },
        { "player",         l_gd_player         },
        { "set_player_pos", l_gd_set_player_pos },
        { "time",           l_gd_time           },
        { nullptr,          nullptr             },
    };
    registerTable(L, "gd", kGd);

    static const luaL_Reg kDraw[] = {
        { "screen_size",   l_draw_screen_size   },
        { "text",          l_draw_text          },
        { "text_gradient", l_draw_text_gradient },
        { "text_rainbow",  l_draw_text_rainbow  },
        { "text_size",   l_draw_text_size   },
        { "rect",        l_draw_rect        },
        { "rect_filled", l_draw_rect_filled },
        { "line",        l_draw_line        },
        { "circle",      l_draw_circle      },
        { nullptr,       nullptr            },
    };
    registerTable(L, "draw", kDraw);

    static const luaL_Reg kStorage[] = {
        { "get",   l_storage_get },
        { "set",   l_storage_set },
        { nullptr, nullptr       },
    };
    registerTable(L, "storage", kStorage);

    static const luaL_Reg kUtils[] = {
        { "clamp", l_utils_clamp },
        { "lerp",  l_utils_lerp  },
        { "round", l_utils_round },
        { nullptr, nullptr       },
    };
    registerTable(L, "utils", kUtils);

    static const luaL_Reg kTabMethods[] = {
        { "toggle",       l_tab_toggle       },
        { "bindable",     l_tab_bindable     },
        { "slider",       l_tab_slider       },
        { "slider_int",   l_tab_slider_int   },
        { "combo",        l_tab_combo        },
        { "button",       l_tab_button       },
        { "label",        l_tab_label        },
        { "input_text",   l_tab_input_text   },
        { "color_picker", l_tab_color_picker },
        { "group",        l_tab_group        },
        { "group_end",    l_tab_group_end    },
        { "separator",    l_tab_separator    },
        { "on_change",    l_tab_on_change    },
        { "text_style",   l_tab_text_style   },
        { "get",          l_tab_get          },
        { "set",          l_tab_set          },
        { nullptr,        nullptr            },
    };

    lua_pushlightuserdata(L, &kTabMetaKey);
    lua_newtable(L);
    lua_newtable(L);
    for (const luaL_Reg* f = kTabMethods; f->name; ++f) {
        lua_pushcfunction(L, f->func);
        lua_setfield(L, -2, f->name);
    }
    lua_setfield(L, -2, "__index");
    lua_pushboolean(L, 0);
    lua_setfield(L, -2, "__metatable");
    lua_rawset(L, LUA_REGISTRYINDEX);

    registerGameApi(L);
    registerMediaApi(L);
    registerUiApi(L);
    registerHooksApi(L);

    registerUtilApi(L);
    registerImguiApi(L);
    registerNodeApi(L);
    registerWorldApi(L);
    registerFsApi(L);
    registerTaskApi(L);
    registerClassApi(L);
}

}
