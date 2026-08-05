
#include "LuaEngine.hpp"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"

extern "C" {
#include <lauxlib.h>
#include <lua.h>
}

#include <cstring>
#include <string>
#include <vector>

namespace nh::lua {
namespace {

constexpr int         kMaxScopes     = 64;
constexpr int         kMaxItems      = 256;
constexpr int         kMaxPoints     = 512;
constexpr int         kMaxPlotPoints = 2048;
constexpr std::size_t kMaxInputLen   = 4096;

enum class Scope {
    Window, Child, Group, TabBar, TabItem, Tree, Table, Popup, Combo,
    StyleColor, StyleVar, Id, Disabled, ItemWidth, Clip
};

struct ScopeEntry {
    Scope kind  = Scope::Window;
    bool  open  = true;
    int   count = 1;
};

std::vector<ScopeEntry> g_scopes;

bool inFrame() { return ImGui::GetCurrentContext() != nullptr; }

void pushScope(lua_State* L, Scope kind, bool open, int count = 1) {
    if ((int)g_scopes.size() >= kMaxScopes) {
        luaL_error(L, "imgui: too many nested begin_* calls (%d max)", kMaxScopes);
        return;
    }

    ScopeEntry entry;
    entry.kind  = kind;
    entry.open  = open;
    entry.count = count;
    g_scopes.push_back(entry);
}

bool popScope(lua_State* L, Scope kind, const char* what) {
    if (g_scopes.empty() || g_scopes.back().kind != kind) {
        luaL_error(L, "imgui.%s() without a matching begin", what);
        return false;
    }

    const ScopeEntry entry = g_scopes.back();
    g_scopes.pop_back();
    return entry.open;
}

void closeScope(const ScopeEntry& entry) {
    switch (entry.kind) {
        case Scope::Window:     ImGui::End();                        break;
        case Scope::Child:      ImGui::EndChild();                   break;
        case Scope::Group:      ImGui::EndGroup();                   break;
        case Scope::TabBar:     if (entry.open) ImGui::EndTabBar();  break;
        case Scope::TabItem:    if (entry.open) ImGui::EndTabItem(); break;
        case Scope::Tree:       if (entry.open) ImGui::TreePop();    break;
        case Scope::Table:      if (entry.open) ImGui::EndTable();   break;
        case Scope::Popup:      if (entry.open) ImGui::EndPopup();   break;
        case Scope::Combo:      if (entry.open) ImGui::EndCombo();   break;
        case Scope::StyleColor: ImGui::PopStyleColor(entry.count);   break;
        case Scope::StyleVar:   ImGui::PopStyleVar(entry.count);     break;
        case Scope::Id:         ImGui::PopID();                      break;
        case Scope::Disabled:   ImGui::EndDisabled();                break;
        case Scope::ItemWidth:  ImGui::PopItemWidth();               break;
        case Scope::Clip:       ImGui::PopClipRect();                break;
    }
}

ImVec4 colorVec(lua_State* L, int first) {
    const float r = (float)luaL_optnumber(L, first + 0, 255.0) / 255.f;
    const float g = (float)luaL_optnumber(L, first + 1, 255.0) / 255.f;
    const float b = (float)luaL_optnumber(L, first + 2, 255.0) / 255.f;
    const float a = (float)luaL_optnumber(L, first + 3, 255.0) / 255.f;
    return ImVec4(r, g, b, a);
}

ImU32 colorU32(lua_State* L, int first) {
    return ImGui::GetColorU32(colorVec(L, first));
}

struct FlagDef {
    const char* name;
    int         value;
};

const FlagDef kWindowFlags[] = {
    { "no_title",             ImGuiWindowFlags_NoTitleBar              },
    { "no_resize",            ImGuiWindowFlags_NoResize               },
    { "no_move",              ImGuiWindowFlags_NoMove                 },
    { "no_scrollbar",         ImGuiWindowFlags_NoScrollbar            },
    { "no_scroll_with_mouse", ImGuiWindowFlags_NoScrollWithMouse      },
    { "no_collapse",          ImGuiWindowFlags_NoCollapse             },
    { "auto_resize",          ImGuiWindowFlags_AlwaysAutoResize       },
    { "no_background",        ImGuiWindowFlags_NoBackground           },
    { "no_saved_settings",    ImGuiWindowFlags_NoSavedSettings        },
    { "no_mouse_inputs",      ImGuiWindowFlags_NoMouseInputs          },
    { "menu_bar",             ImGuiWindowFlags_MenuBar                },
    { "horizontal_scroll",    ImGuiWindowFlags_HorizontalScrollbar    },
    { "no_focus_on_appear",   ImGuiWindowFlags_NoFocusOnAppearing     },
    { "no_bring_to_front",    ImGuiWindowFlags_NoBringToFrontOnFocus  },
    { "vertical_scrollbar",   ImGuiWindowFlags_AlwaysVerticalScrollbar},
    { "no_nav",               ImGuiWindowFlags_NoNav                  },
    { "no_decoration",        ImGuiWindowFlags_NoDecoration           },
    { "no_inputs",            ImGuiWindowFlags_NoInputs               },
    { nullptr,                0                                       },
};

const FlagDef kTableFlags[] = {
    { "resizable",     ImGuiTableFlags_Resizable          },
    { "reorderable",   ImGuiTableFlags_Reorderable        },
    { "hideable",      ImGuiTableFlags_Hideable           },
    { "sortable",      ImGuiTableFlags_Sortable           },
    { "row_bg",        ImGuiTableFlags_RowBg              },
    { "borders",       ImGuiTableFlags_Borders            },
    { "borders_inner", ImGuiTableFlags_BordersInner       },
    { "borders_outer", ImGuiTableFlags_BordersOuter       },
    { "scroll_x",      ImGuiTableFlags_ScrollX            },
    { "scroll_y",      ImGuiTableFlags_ScrollY            },
    { "stretch",       ImGuiTableFlags_SizingStretchProp  },
    { "fixed",         ImGuiTableFlags_SizingFixedFit     },
    { "no_clip",       ImGuiTableFlags_NoClip             },
    { nullptr,         0                                   },
};

int flagValue(lua_State* L, const char* name, const FlagDef* defs) {
    for (const FlagDef* def = defs; def->name; ++def)
        if (std::strcmp(def->name, name) == 0) return def->value;

    luaL_error(L, "unknown imgui flag '%s'", name);
    return 0;
}

int parseFlags(lua_State* L, int first, const FlagDef* defs) {
    int flags = 0;
    const int top = lua_gettop(L);

    for (int i = first; i <= top; ++i) {
        if (lua_isnoneornil(L, i)) continue;

        if (lua_istable(L, i)) {
            const int count = (int)lua_objlen(L, i);
            for (int k = 1; k <= count; ++k) {
                lua_rawgeti(L, i, k);
                if (lua_isstring(L, -1))
                    flags |= flagValue(L, lua_tostring(L, -1), defs);
                lua_pop(L, 1);
            }
            continue;
        }

        if (lua_isstring(L, i))
            flags |= flagValue(L, lua_tostring(L, i), defs);
    }

    return flags;
}

void readItems(lua_State* L, int index, std::vector<std::string>& out) {
    luaL_checktype(L, index, LUA_TTABLE);

    const int count = (int)lua_objlen(L, index);
    if (count <= 0)      luaL_error(L, "the item list is empty");
    if (count > kMaxItems) luaL_error(L, "too many items (%d max)", kMaxItems);

    out.reserve((std::size_t)count);
    for (int i = 1; i <= count; ++i) {
        lua_rawgeti(L, index, i);
        out.push_back(lua_isstring(L, -1) ? lua_tostring(L, -1) : "");
        lua_pop(L, 1);
    }
}

void readPoints(lua_State* L, int index, std::vector<ImVec2>& out) {
    luaL_checktype(L, index, LUA_TTABLE);

    const int count = (int)lua_objlen(L, index);
    if (count < 4)  luaL_error(L, "a polyline needs at least two x,y pairs");
    if (count / 2 > kMaxPoints)
        luaL_error(L, "too many points (%d max)", kMaxPoints);

    for (int i = 1; i + 1 <= count; i += 2) {
        lua_rawgeti(L, index, i);
        lua_rawgeti(L, index, i + 1);

        out.push_back(ImVec2((float)lua_tonumber(L, -2), (float)lua_tonumber(L, -1)));
        lua_pop(L, 2);
    }
}

int l_begin_window(lua_State* L) {
    const char* name = luaL_checkstring(L, 1);
    if (!inFrame()) { lua_pushboolean(L, 0); return 1; }

    const int flags = parseFlags(L, 2, kWindowFlags);
    const bool visible = ImGui::Begin(name, nullptr, flags);

    pushScope(L, Scope::Window, true);

    lua_pushboolean(L, visible ? 1 : 0);
    return 1;
}

int l_end_window(lua_State* L) {
    if (popScope(L, Scope::Window, "end_window")) ImGui::End();
    return 0;
}

int l_begin_child(lua_State* L) {
    const char* id = luaL_checkstring(L, 1);
    const float w  = (float)luaL_optnumber(L, 2, 0.0);
    const float h  = (float)luaL_optnumber(L, 3, 0.0);

    if (!inFrame()) { lua_pushboolean(L, 0); return 1; }

    const bool border  = lua_isnoneornil(L, 4) ? false : lua_toboolean(L, 4) != 0;
    const bool visible = ImGui::BeginChild(id, ImVec2(w, h), border);

    pushScope(L, Scope::Child, true);

    lua_pushboolean(L, visible ? 1 : 0);
    return 1;
}

int l_end_child(lua_State* L) {
    if (popScope(L, Scope::Child, "end_child")) ImGui::EndChild();
    return 0;
}

int l_begin_group(lua_State* L) {
    if (!inFrame()) return 0;

    ImGui::BeginGroup();
    pushScope(L, Scope::Group, true);
    return 0;
}

int l_end_group(lua_State* L) {
    if (popScope(L, Scope::Group, "end_group")) ImGui::EndGroup();
    return 0;
}

int l_set_next_pos(lua_State* L) {
    const float x = (float)luaL_checknumber(L, 1);
    const float y = (float)luaL_checknumber(L, 2);

    const bool always = lua_toboolean(L, 3) != 0;
    if (inFrame())
        ImGui::SetNextWindowPos(ImVec2(x, y),
            always ? ImGuiCond_Always : ImGuiCond_FirstUseEver);
    return 0;
}

int l_set_next_size(lua_State* L) {
    const float w = (float)luaL_checknumber(L, 1);
    const float h = (float)luaL_checknumber(L, 2);

    const bool always = lua_toboolean(L, 3) != 0;
    if (inFrame())
        ImGui::SetNextWindowSize(ImVec2(w, h),
            always ? ImGuiCond_Always : ImGuiCond_FirstUseEver);
    return 0;
}

int l_set_next_bg_alpha(lua_State* L) {
    if (inFrame()) ImGui::SetNextWindowBgAlpha((float)luaL_checknumber(L, 1));
    return 0;
}

int l_window_pos(lua_State* L) {
    const ImVec2 pos = inFrame() ? ImGui::GetWindowPos() : ImVec2(0.f, 0.f);
    lua_pushnumber(L, pos.x);
    lua_pushnumber(L, pos.y);
    return 2;
}

int l_window_size(lua_State* L) {
    const ImVec2 size = inFrame() ? ImGui::GetWindowSize() : ImVec2(0.f, 0.f);
    lua_pushnumber(L, size.x);
    lua_pushnumber(L, size.y);
    return 2;
}

int l_content_avail(lua_State* L) {
    const ImVec2 size = inFrame() ? ImGui::GetContentRegionAvail() : ImVec2(0.f, 0.f);
    lua_pushnumber(L, size.x);
    lua_pushnumber(L, size.y);
    return 2;
}

int l_cursor_pos(lua_State* L) {
    const ImVec2 pos = inFrame() ? ImGui::GetCursorPos() : ImVec2(0.f, 0.f);
    lua_pushnumber(L, pos.x);
    lua_pushnumber(L, pos.y);
    return 2;
}

int l_set_cursor_pos(lua_State* L) {
    const float x = (float)luaL_checknumber(L, 1);
    const float y = (float)luaL_checknumber(L, 2);

    if (inFrame()) ImGui::SetCursorPos(ImVec2(x, y));
    return 0;
}

int l_cursor_screen_pos(lua_State* L) {
    const ImVec2 pos = inFrame() ? ImGui::GetCursorScreenPos() : ImVec2(0.f, 0.f);
    lua_pushnumber(L, pos.x);
    lua_pushnumber(L, pos.y);
    return 2;
}

int l_same_line(lua_State* L) {
    const float offset  = (float)luaL_optnumber(L, 1, 0.0);
    const float spacing = (float)luaL_optnumber(L, 2, -1.0);

    if (inFrame()) ImGui::SameLine(offset, spacing);
    return 0;
}

int l_spacing(lua_State*)   { if (inFrame()) ImGui::Spacing();   return 0; }
int l_new_line(lua_State*)  { if (inFrame()) ImGui::NewLine();   return 0; }
int l_separator(lua_State*) { if (inFrame()) ImGui::Separator(); return 0; }

int l_indent(lua_State* L) {
    if (inFrame()) ImGui::Indent((float)luaL_optnumber(L, 1, 0.0));
    return 0;
}

int l_unindent(lua_State* L) {
    if (inFrame()) ImGui::Unindent((float)luaL_optnumber(L, 1, 0.0));
    return 0;
}

int l_dummy(lua_State* L) {
    const float w = (float)luaL_checknumber(L, 1);
    const float h = (float)luaL_checknumber(L, 2);

    if (inFrame()) ImGui::Dummy(ImVec2(w, h));
    return 0;
}

int l_set_next_width(lua_State* L) {
    if (inFrame()) ImGui::SetNextItemWidth((float)luaL_checknumber(L, 1));
    return 0;
}

int l_push_item_width(lua_State* L) {
    if (!inFrame()) return 0;

    ImGui::PushItemWidth((float)luaL_checknumber(L, 1));
    pushScope(L, Scope::ItemWidth, true);
    return 0;
}

int l_pop_item_width(lua_State* L) {
    if (popScope(L, Scope::ItemWidth, "pop_item_width")) ImGui::PopItemWidth();
    return 0;
}

int l_push_id(lua_State* L) {
    if (!inFrame()) return 0;

    if (lua_isnumber(L, 1)) ImGui::PushID((int)lua_tointeger(L, 1));
    else                    ImGui::PushID(luaL_checkstring(L, 1));

    pushScope(L, Scope::Id, true);
    return 0;
}

int l_pop_id(lua_State* L) {
    if (popScope(L, Scope::Id, "pop_id")) ImGui::PopID();
    return 0;
}

int l_begin_disabled(lua_State* L) {
    if (!inFrame()) return 0;

    const bool disabled = lua_isnoneornil(L, 1) ? true : lua_toboolean(L, 1) != 0;
    ImGui::BeginDisabled(disabled);
    pushScope(L, Scope::Disabled, true);
    return 0;
}

int l_end_disabled(lua_State* L) {
    if (popScope(L, Scope::Disabled, "end_disabled")) ImGui::EndDisabled();
    return 0;
}

struct StyleColorDef {
    const char*  name;
    ImGuiCol     value;
};

const StyleColorDef kStyleColors[] = {
    { "text",              ImGuiCol_Text                },
    { "text_disabled",     ImGuiCol_TextDisabled        },
    { "window_bg",         ImGuiCol_WindowBg            },
    { "child_bg",          ImGuiCol_ChildBg             },
    { "popup_bg",          ImGuiCol_PopupBg             },
    { "border",            ImGuiCol_Border              },
    { "frame_bg",          ImGuiCol_FrameBg             },
    { "frame_bg_hovered",  ImGuiCol_FrameBgHovered      },
    { "frame_bg_active",   ImGuiCol_FrameBgActive       },
    { "title_bg",          ImGuiCol_TitleBg             },
    { "title_bg_active",   ImGuiCol_TitleBgActive       },
    { "menu_bar_bg",       ImGuiCol_MenuBarBg           },
    { "scrollbar_bg",      ImGuiCol_ScrollbarBg         },
    { "scrollbar_grab",    ImGuiCol_ScrollbarGrab       },
    { "check_mark",        ImGuiCol_CheckMark           },
    { "slider_grab",       ImGuiCol_SliderGrab          },
    { "slider_grab_active",ImGuiCol_SliderGrabActive    },
    { "button",            ImGuiCol_Button              },
    { "button_hovered",    ImGuiCol_ButtonHovered       },
    { "button_active",     ImGuiCol_ButtonActive        },
    { "header",            ImGuiCol_Header              },
    { "header_hovered",    ImGuiCol_HeaderHovered       },
    { "header_active",     ImGuiCol_HeaderActive        },
    { "separator",         ImGuiCol_Separator           },
    { "resize_grip",       ImGuiCol_ResizeGrip          },
    { "plot_lines",        ImGuiCol_PlotLines           },
    { "plot_histogram",    ImGuiCol_PlotHistogram       },
    { "table_header_bg",   ImGuiCol_TableHeaderBg       },
    { "table_row_bg",      ImGuiCol_TableRowBg          },
    { "text_selected_bg",  ImGuiCol_TextSelectedBg      },
    { nullptr,             ImGuiCol_Text                },
};

struct StyleVarDef {
    const char*   name;
    ImGuiStyleVar value;
    bool          vec2;
};

const StyleVarDef kStyleVars[] = {
    { "alpha",              ImGuiStyleVar_Alpha,              false },
    { "disabled_alpha",     ImGuiStyleVar_DisabledAlpha,      false },
    { "window_padding",     ImGuiStyleVar_WindowPadding,      true  },
    { "window_rounding",    ImGuiStyleVar_WindowRounding,     false },
    { "window_border",      ImGuiStyleVar_WindowBorderSize,   false },
    { "window_min_size",    ImGuiStyleVar_WindowMinSize,      true  },
    { "child_rounding",     ImGuiStyleVar_ChildRounding,      false },
    { "child_border",       ImGuiStyleVar_ChildBorderSize,    false },
    { "popup_rounding",     ImGuiStyleVar_PopupRounding,      false },
    { "popup_border",       ImGuiStyleVar_PopupBorderSize,    false },
    { "frame_padding",      ImGuiStyleVar_FramePadding,       true  },
    { "frame_rounding",     ImGuiStyleVar_FrameRounding,      false },
    { "frame_border",       ImGuiStyleVar_FrameBorderSize,    false },
    { "item_spacing",       ImGuiStyleVar_ItemSpacing,        true  },
    { "item_inner_spacing", ImGuiStyleVar_ItemInnerSpacing,   true  },
    { "indent",             ImGuiStyleVar_IndentSpacing,      false },
    { "scrollbar_size",     ImGuiStyleVar_ScrollbarSize,      false },
    { "scrollbar_rounding", ImGuiStyleVar_ScrollbarRounding,  false },
    { "grab_min_size",      ImGuiStyleVar_GrabMinSize,        false },
    { "grab_rounding",      ImGuiStyleVar_GrabRounding,       false },
    { "tab_rounding",       ImGuiStyleVar_TabRounding,        false },
    { "button_align",       ImGuiStyleVar_ButtonTextAlign,    true  },
    { "selectable_align",   ImGuiStyleVar_SelectableTextAlign,true  },
    { nullptr,              ImGuiStyleVar_Alpha,              false },
};

int l_push_style_color(lua_State* L) {
    const char* name = luaL_checkstring(L, 1);

    const StyleColorDef* def = kStyleColors;
    for (; def->name; ++def)
        if (std::strcmp(def->name, name) == 0) break;

    if (!def->name)
        return luaL_error(L, "unknown style colour '%s'", name);

    if (!inFrame()) return 0;

    ImGui::PushStyleColor(def->value, colorVec(L, 2));
    pushScope(L, Scope::StyleColor, true, 1);
    return 0;
}

int l_pop_style_color(lua_State* L) {
    if (popScope(L, Scope::StyleColor, "pop_style_color")) ImGui::PopStyleColor(1);
    return 0;
}

int l_push_style_var(lua_State* L) {
    const char* name = luaL_checkstring(L, 1);

    const StyleVarDef* def = kStyleVars;
    for (; def->name; ++def)
        if (std::strcmp(def->name, name) == 0) break;

    if (!def->name)
        return luaL_error(L, "unknown style var '%s'", name);

    const float first = (float)luaL_checknumber(L, 2);

    if (def->vec2 && lua_isnoneornil(L, 3))
        return luaL_error(L, "style var '%s' needs two numbers", name);

    if (!inFrame()) return 0;

    if (def->vec2)
        ImGui::PushStyleVar(def->value, ImVec2(first, (float)luaL_checknumber(L, 3)));
    else
        ImGui::PushStyleVar(def->value, first);

    pushScope(L, Scope::StyleVar, true, 1);
    return 0;
}

int l_pop_style_var(lua_State* L) {
    if (popScope(L, Scope::StyleVar, "pop_style_var")) ImGui::PopStyleVar(1);
    return 0;
}

int l_text(lua_State* L) {
    const char* text = luaL_checkstring(L, 1);
    if (inFrame()) ImGui::TextUnformatted(text);
    return 0;
}

int l_text_colored(lua_State* L) {
    const char* text = luaL_checkstring(L, 1);
    if (!inFrame()) return 0;

    ImGui::PushStyleColor(ImGuiCol_Text, colorVec(L, 2));
    ImGui::TextUnformatted(text);
    ImGui::PopStyleColor();
    return 0;
}

int l_text_disabled(lua_State* L) {
    const char* text = luaL_checkstring(L, 1);
    if (!inFrame()) return 0;

    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
    ImGui::TextUnformatted(text);
    ImGui::PopStyleColor();
    return 0;
}

int l_text_wrapped(lua_State* L) {
    const char* text = luaL_checkstring(L, 1);
    if (!inFrame()) return 0;

    ImGui::PushTextWrapPos(0.f);
    ImGui::TextUnformatted(text);
    ImGui::PopTextWrapPos();
    return 0;
}

int l_bullet_text(lua_State* L) {
    const char* text = luaL_checkstring(L, 1);
    if (!inFrame()) return 0;

    ImGui::Bullet();
    ImGui::TextUnformatted(text);
    return 0;
}

int l_label_text(lua_State* L) {
    const char* label = luaL_checkstring(L, 1);
    const char* value = luaL_checkstring(L, 2);

    if (inFrame()) ImGui::LabelText(label, "%s", value);
    return 0;
}

int l_text_size(lua_State* L) {
    const char* text = luaL_checkstring(L, 1);

    const ImVec2 size = inFrame() ? ImGui::CalcTextSize(text) : ImVec2(0.f, 0.f);
    lua_pushnumber(L, size.x);
    lua_pushnumber(L, size.y);
    return 2;
}

int l_button(lua_State* L) {
    const char* label = luaL_checkstring(L, 1);
    const float w     = (float)luaL_optnumber(L, 2, 0.0);
    const float h     = (float)luaL_optnumber(L, 3, 0.0);

    lua_pushboolean(L, inFrame() && ImGui::Button(label, ImVec2(w, h)) ? 1 : 0);
    return 1;
}

int l_small_button(lua_State* L) {
    const char* label = luaL_checkstring(L, 1);
    lua_pushboolean(L, inFrame() && ImGui::SmallButton(label) ? 1 : 0);
    return 1;
}

int l_invisible_button(lua_State* L) {
    const char* id = luaL_checkstring(L, 1);
    const float w  = (float)luaL_checknumber(L, 2);
    const float h  = (float)luaL_checknumber(L, 3);

    lua_pushboolean(L, inFrame() && ImGui::InvisibleButton(id, ImVec2(w, h)) ? 1 : 0);
    return 1;
}

int l_checkbox(lua_State* L) {
    const char* label = luaL_checkstring(L, 1);
    bool        value = lua_toboolean(L, 2) != 0;

    const bool changed = inFrame() && ImGui::Checkbox(label, &value);

    lua_pushboolean(L, value ? 1 : 0);
    lua_pushboolean(L, changed ? 1 : 0);
    return 2;
}

int l_radio(lua_State* L) {
    const char* label  = luaL_checkstring(L, 1);
    const bool  active = lua_toboolean(L, 2) != 0;

    lua_pushboolean(L, inFrame() && ImGui::RadioButton(label, active) ? 1 : 0);
    return 1;
}

int l_slider(lua_State* L) {
    const char* label = luaL_checkstring(L, 1);
    float       value = (float)luaL_checknumber(L, 2);
    const float min   = (float)luaL_optnumber(L, 3, 0.0);
    const float max   = (float)luaL_optnumber(L, 4, 1.0);
    const char* fmt   = luaL_optstring(L, 5, "%.2f");

    const bool changed = inFrame() && ImGui::SliderFloat(label, &value, min, max, fmt);

    lua_pushnumber(L, value);
    lua_pushboolean(L, changed ? 1 : 0);
    return 2;
}

int l_slider_int(lua_State* L) {
    const char* label = luaL_checkstring(L, 1);
    int         value = (int)luaL_checkinteger(L, 2);
    const int   min   = (int)luaL_optinteger(L, 3, 0);
    const int   max   = (int)luaL_optinteger(L, 4, 100);

    const bool changed = inFrame() && ImGui::SliderInt(label, &value, min, max);

    lua_pushinteger(L, value);
    lua_pushboolean(L, changed ? 1 : 0);
    return 2;
}

int l_drag(lua_State* L) {
    const char* label = luaL_checkstring(L, 1);
    float       value = (float)luaL_checknumber(L, 2);
    const float speed = (float)luaL_optnumber(L, 3, 1.0);
    const float min   = (float)luaL_optnumber(L, 4, 0.0);
    const float max   = (float)luaL_optnumber(L, 5, 0.0);

    const bool changed = inFrame() && ImGui::DragFloat(label, &value, speed, min, max);

    lua_pushnumber(L, value);
    lua_pushboolean(L, changed ? 1 : 0);
    return 2;
}

int l_drag_int(lua_State* L) {
    const char* label = luaL_checkstring(L, 1);
    int         value = (int)luaL_checkinteger(L, 2);
    const float speed = (float)luaL_optnumber(L, 3, 1.0);
    const int   min   = (int)luaL_optinteger(L, 4, 0);
    const int   max   = (int)luaL_optinteger(L, 5, 0);

    const bool changed = inFrame() && ImGui::DragInt(label, &value, speed, min, max);

    lua_pushinteger(L, value);
    lua_pushboolean(L, changed ? 1 : 0);
    return 2;
}

std::vector<char> inputBuffer(lua_State* L, int index, int capIndex) {
    std::size_t len = 0;
    const char* text = lua_isnoneornil(L, index) ? "" : luaL_checklstring(L, index, &len);

    std::size_t cap = (std::size_t)luaL_optinteger(L, capIndex, 256);
    if (cap < 16)           cap = 16;
    if (cap > kMaxInputLen) cap = kMaxInputLen;
    if (len > cap)          len = cap;

    std::vector<char> buffer(cap + 1, '\0');
    if (len) std::memcpy(buffer.data(), text, len);
    return buffer;
}

int l_input_text(lua_State* L) {
    const char* label = luaL_checkstring(L, 1);
    std::vector<char> buffer = inputBuffer(L, 2, 3);

    const bool changed = inFrame()
        && ImGui::InputText(label, buffer.data(), buffer.size());

    lua_pushstring(L, buffer.data());
    lua_pushboolean(L, changed ? 1 : 0);
    return 2;
}

int l_input_text_multiline(lua_State* L) {
    const char* label = luaL_checkstring(L, 1);
    std::vector<char> buffer = inputBuffer(L, 2, 5);

    const float w = (float)luaL_optnumber(L, 3, 0.0);
    const float h = (float)luaL_optnumber(L, 4, 0.0);

    const bool changed = inFrame()
        && ImGui::InputTextMultiline(label, buffer.data(), buffer.size(), ImVec2(w, h));

    lua_pushstring(L, buffer.data());
    lua_pushboolean(L, changed ? 1 : 0);
    return 2;
}

int l_input_number(lua_State* L) {
    const char* label = luaL_checkstring(L, 1);
    float       value = (float)luaL_checknumber(L, 2);
    const float step  = (float)luaL_optnumber(L, 3, 0.0);

    const bool changed = inFrame()
        && ImGui::InputFloat(label, &value, step, step * 10.f, "%.3f");

    lua_pushnumber(L, value);
    lua_pushboolean(L, changed ? 1 : 0);
    return 2;
}

int l_combo(lua_State* L) {
    const char* label = luaL_checkstring(L, 1);
    int         index = (int)luaL_checkinteger(L, 2);

    std::vector<std::string> items;
    readItems(L, 3, items);

    std::vector<const char*> pointers;
    pointers.reserve(items.size());
    for (const auto& item : items) pointers.push_back(item.c_str());

    int zeroBased = index - 1;
    if (zeroBased < 0) zeroBased = 0;
    if (zeroBased >= (int)pointers.size()) zeroBased = (int)pointers.size() - 1;

    const bool changed = inFrame()
        && ImGui::Combo(label, &zeroBased, pointers.data(), (int)pointers.size());

    lua_pushinteger(L, zeroBased + 1);
    lua_pushboolean(L, changed ? 1 : 0);
    return 2;
}

int l_listbox(lua_State* L) {
    const char* label = luaL_checkstring(L, 1);
    int         index = (int)luaL_checkinteger(L, 2);

    std::vector<std::string> items;
    readItems(L, 3, items);

    std::vector<const char*> pointers;
    pointers.reserve(items.size());
    for (const auto& item : items) pointers.push_back(item.c_str());

    const int height = (int)luaL_optinteger(L, 4, -1);

    int zeroBased = index - 1;
    if (zeroBased < 0) zeroBased = 0;
    if (zeroBased >= (int)pointers.size()) zeroBased = (int)pointers.size() - 1;

    const bool changed = inFrame()
        && ImGui::ListBox(label, &zeroBased, pointers.data(), (int)pointers.size(), height);

    lua_pushinteger(L, zeroBased + 1);
    lua_pushboolean(L, changed ? 1 : 0);
    return 2;
}

int l_color_edit(lua_State* L) {
    const char* label = luaL_checkstring(L, 1);

    float color[4] = {
        (float)luaL_optnumber(L, 2, 255.0) / 255.f,
        (float)luaL_optnumber(L, 3, 255.0) / 255.f,
        (float)luaL_optnumber(L, 4, 255.0) / 255.f,
        (float)luaL_optnumber(L, 5, 255.0) / 255.f,
    };

    const bool changed = inFrame() && ImGui::ColorEdit4(label, color,
        ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_AlphaPreviewHalf);

    for (int i = 0; i < 4; ++i) lua_pushnumber(L, color[i] * 255.f);
    lua_pushboolean(L, changed ? 1 : 0);
    return 5;
}

int l_progress_bar(lua_State* L) {
    const float fraction = (float)luaL_checknumber(L, 1);
    const float w        = (float)luaL_optnumber(L, 2, -1.0);
    const float h        = (float)luaL_optnumber(L, 3, 0.0);
    const char* overlay  = luaL_optstring(L, 4, nullptr);

    if (inFrame()) ImGui::ProgressBar(fraction, ImVec2(w, h), overlay);
    return 0;
}

int l_selectable(lua_State* L) {
    const char* label    = luaL_checkstring(L, 1);
    const bool  selected = lua_toboolean(L, 2) != 0;
    const float w        = (float)luaL_optnumber(L, 3, 0.0);
    const float h        = (float)luaL_optnumber(L, 4, 0.0);

    lua_pushboolean(L, inFrame()
        && ImGui::Selectable(label, selected, 0, ImVec2(w, h)) ? 1 : 0);
    return 1;
}

int l_collapsing_header(lua_State* L) {
    const char* label = luaL_checkstring(L, 1);
    lua_pushboolean(L, inFrame() && ImGui::CollapsingHeader(label) ? 1 : 0);
    return 1;
}

int l_tree_node(lua_State* L) {
    const char* label = luaL_checkstring(L, 1);
    if (!inFrame()) { lua_pushboolean(L, 0); return 1; }

    const bool open = ImGui::TreeNode(label);
    pushScope(L, Scope::Tree, open);

    lua_pushboolean(L, open ? 1 : 0);
    return 1;
}

int l_tree_pop(lua_State* L) {
    if (popScope(L, Scope::Tree, "tree_pop")) ImGui::TreePop();
    return 0;
}

int l_begin_tab_bar(lua_State* L) {
    const char* id = luaL_checkstring(L, 1);
    if (!inFrame()) { lua_pushboolean(L, 0); return 1; }

    const bool open = ImGui::BeginTabBar(id);
    pushScope(L, Scope::TabBar, open);

    lua_pushboolean(L, open ? 1 : 0);
    return 1;
}

int l_end_tab_bar(lua_State* L) {
    if (popScope(L, Scope::TabBar, "end_tab_bar")) ImGui::EndTabBar();
    return 0;
}

int l_begin_tab_item(lua_State* L) {
    const char* label = luaL_checkstring(L, 1);
    if (!inFrame()) { lua_pushboolean(L, 0); return 1; }

    const bool open = ImGui::BeginTabItem(label);
    pushScope(L, Scope::TabItem, open);

    lua_pushboolean(L, open ? 1 : 0);
    return 1;
}

int l_end_tab_item(lua_State* L) {
    if (popScope(L, Scope::TabItem, "end_tab_item")) ImGui::EndTabItem();
    return 0;
}

int l_begin_combo(lua_State* L) {
    const char* label   = luaL_checkstring(L, 1);
    const char* preview = luaL_optstring(L, 2, "");

    if (!inFrame()) { lua_pushboolean(L, 0); return 1; }

    const bool open = ImGui::BeginCombo(label, preview);
    pushScope(L, Scope::Combo, open);

    lua_pushboolean(L, open ? 1 : 0);
    return 1;
}

int l_end_combo(lua_State* L) {
    if (popScope(L, Scope::Combo, "end_combo")) ImGui::EndCombo();
    return 0;
}

int l_begin_table(lua_State* L) {
    const char* id      = luaL_checkstring(L, 1);
    const int   columns = (int)luaL_checkinteger(L, 2);

    if (columns < 1 || columns > 32)
        return luaL_error(L, "a table needs between 1 and 32 columns");

    if (!inFrame()) { lua_pushboolean(L, 0); return 1; }

    const int  flags = parseFlags(L, 3, kTableFlags);
    const bool open  = ImGui::BeginTable(id, columns, flags);
    pushScope(L, Scope::Table, open);

    lua_pushboolean(L, open ? 1 : 0);
    return 1;
}

int l_end_table(lua_State* L) {
    if (popScope(L, Scope::Table, "end_table")) ImGui::EndTable();
    return 0;
}

int l_table_setup_column(lua_State* L) {
    const char* label = luaL_checkstring(L, 1);
    const float width = (float)luaL_optnumber(L, 2, 0.0);

    if (inFrame()) ImGui::TableSetupColumn(label, 0, width);
    return 0;
}

int l_table_headers_row(lua_State*) {
    if (inFrame()) ImGui::TableHeadersRow();
    return 0;
}

int l_table_next_row(lua_State* L) {
    const float height = (float)luaL_optnumber(L, 1, 0.0);
    if (inFrame()) ImGui::TableNextRow(0, height);
    return 0;
}

int l_table_next_column(lua_State* L) {
    lua_pushboolean(L, inFrame() && ImGui::TableNextColumn() ? 1 : 0);
    return 1;
}

int l_table_set_column(lua_State* L) {
    const int index = (int)luaL_checkinteger(L, 1);
    lua_pushboolean(L, inFrame() && ImGui::TableSetColumnIndex(index - 1) ? 1 : 0);
    return 1;
}

int l_open_popup(lua_State* L) {
    const char* id = luaL_checkstring(L, 1);
    if (inFrame()) ImGui::OpenPopup(id);
    return 0;
}

int l_begin_popup(lua_State* L) {
    const char* id = luaL_checkstring(L, 1);
    if (!inFrame()) { lua_pushboolean(L, 0); return 1; }

    const bool open = ImGui::BeginPopup(id);
    pushScope(L, Scope::Popup, open);

    lua_pushboolean(L, open ? 1 : 0);
    return 1;
}

int l_begin_popup_modal(lua_State* L) {
    const char* id = luaL_checkstring(L, 1);
    if (!inFrame()) { lua_pushboolean(L, 0); return 1; }

    const bool open = ImGui::BeginPopupModal(id, nullptr,
        ImGuiWindowFlags_AlwaysAutoResize);
    pushScope(L, Scope::Popup, open);

    lua_pushboolean(L, open ? 1 : 0);
    return 1;
}

int l_end_popup(lua_State* L) {
    if (popScope(L, Scope::Popup, "end_popup")) ImGui::EndPopup();
    return 0;
}

int l_close_popup(lua_State*) {
    if (inFrame()) ImGui::CloseCurrentPopup();
    return 0;
}

int l_set_tooltip(lua_State* L) {
    const char* text = luaL_checkstring(L, 1);
    if (inFrame()) ImGui::SetTooltip("%s", text);
    return 0;
}

void readPlotValues(lua_State* L, int index, std::vector<float>& out) {
    luaL_checktype(L, index, LUA_TTABLE);

    const int count = (int)lua_objlen(L, index);
    if (count <= 0) luaL_error(L, "the plot has no values");
    if (count > kMaxPlotPoints)
        luaL_error(L, "too many plot values (%d max)", kMaxPlotPoints);

    out.reserve((std::size_t)count);
    for (int i = 1; i <= count; ++i) {
        lua_rawgeti(L, index, i);
        out.push_back((float)lua_tonumber(L, -1));
        lua_pop(L, 1);
    }
}

int l_plot_lines(lua_State* L) {
    const char* label = luaL_checkstring(L, 1);

    std::vector<float> values;
    readPlotValues(L, 2, values);

    const float w   = (float)luaL_optnumber(L, 3, 0.0);
    const float h   = (float)luaL_optnumber(L, 4, 60.0);
    const float min = (float)luaL_optnumber(L, 5, (double)FLT_MAX);
    const float max = (float)luaL_optnumber(L, 6, (double)FLT_MAX);

    if (inFrame())
        ImGui::PlotLines(label, values.data(), (int)values.size(), 0, nullptr,
                         min, max, ImVec2(w, h));
    return 0;
}

int l_plot_histogram(lua_State* L) {
    const char* label = luaL_checkstring(L, 1);

    std::vector<float> values;
    readPlotValues(L, 2, values);

    const float w = (float)luaL_optnumber(L, 3, 0.0);
    const float h = (float)luaL_optnumber(L, 4, 60.0);

    if (inFrame())
        ImGui::PlotHistogram(label, values.data(), (int)values.size(), 0, nullptr,
                             FLT_MAX, FLT_MAX, ImVec2(w, h));
    return 0;
}

int l_is_item_hovered(lua_State* L) {
    lua_pushboolean(L, inFrame() && ImGui::IsItemHovered() ? 1 : 0);
    return 1;
}

int l_is_item_clicked(lua_State* L) {
    const int button = (int)luaL_optinteger(L, 1, 0);
    lua_pushboolean(L, inFrame() && ImGui::IsItemClicked(button) ? 1 : 0);
    return 1;
}

int l_is_item_active(lua_State* L) {
    lua_pushboolean(L, inFrame() && ImGui::IsItemActive() ? 1 : 0);
    return 1;
}

int l_is_window_hovered(lua_State* L) {
    lua_pushboolean(L, inFrame() && ImGui::IsWindowHovered() ? 1 : 0);
    return 1;
}

int l_is_window_focused(lua_State* L) {
    lua_pushboolean(L, inFrame() && ImGui::IsWindowFocused() ? 1 : 0);
    return 1;
}

int l_frame_height(lua_State* L) {
    lua_pushnumber(L, inFrame() ? ImGui::GetFrameHeight() : 0.f);
    return 1;
}

int l_font_size(lua_State* L) {
    lua_pushnumber(L, inFrame() ? ImGui::GetFontSize() : 0.f);
    return 1;
}

int l_style_color(lua_State* L) {
    const char* name = luaL_checkstring(L, 1);

    const StyleColorDef* def = kStyleColors;
    for (; def->name; ++def)
        if (std::strcmp(def->name, name) == 0) break;

    if (!def->name)
        return luaL_error(L, "unknown style colour '%s'", name);

    if (!inFrame()) return 0;

    const ImVec4 color = ImGui::GetStyle().Colors[def->value];
    lua_pushnumber(L, color.x * 255.f);
    lua_pushnumber(L, color.y * 255.f);
    lua_pushnumber(L, color.z * 255.f);
    lua_pushnumber(L, color.w * 255.f);
    return 4;
}

ImDrawList* list() { return ImGui::GetForegroundDrawList(); }

int l_draw_circle_filled(lua_State* L) {
    const float x = (float)luaL_checknumber(L, 1);
    const float y = (float)luaL_checknumber(L, 2);
    const float r = (float)luaL_checknumber(L, 3);

    if (inFrame()) list()->AddCircleFilled(ImVec2(x, y), r, colorU32(L, 4), 0);
    return 0;
}

int l_draw_rect_rounded(lua_State* L) {
    const float x = (float)luaL_checknumber(L, 1);
    const float y = (float)luaL_checknumber(L, 2);
    const float w = (float)luaL_checknumber(L, 3);
    const float h = (float)luaL_checknumber(L, 4);
    const float r = (float)luaL_checknumber(L, 5);

    if (inFrame())
        list()->AddRect(ImVec2(x, y), ImVec2(x + w, y + h), colorU32(L, 6), r, 0,
                        (float)luaL_optnumber(L, 10, 1.0));
    return 0;
}

int l_draw_rect_rounded_filled(lua_State* L) {
    const float x = (float)luaL_checknumber(L, 1);
    const float y = (float)luaL_checknumber(L, 2);
    const float w = (float)luaL_checknumber(L, 3);
    const float h = (float)luaL_checknumber(L, 4);
    const float r = (float)luaL_checknumber(L, 5);

    if (inFrame())
        list()->AddRectFilled(ImVec2(x, y), ImVec2(x + w, y + h), colorU32(L, 6), r);
    return 0;
}

int l_draw_rect_gradient(lua_State* L) {
    const float x = (float)luaL_checknumber(L, 1);
    const float y = (float)luaL_checknumber(L, 2);
    const float w = (float)luaL_checknumber(L, 3);
    const float h = (float)luaL_checknumber(L, 4);

    const ImU32 first  = colorU32(L, 5);
    const ImU32 second = colorU32(L, 9);
    const bool  vertical = lua_toboolean(L, 13) != 0;

    if (!inFrame()) return 0;

    const ImVec2 min(x, y);
    const ImVec2 max(x + w, y + h);

    if (vertical)
        list()->AddRectFilledMultiColor(min, max, first, first, second, second);
    else
        list()->AddRectFilledMultiColor(min, max, first, second, second, first);
    return 0;
}

int l_draw_triangle(lua_State* L) {
    const ImVec2 a((float)luaL_checknumber(L, 1), (float)luaL_checknumber(L, 2));
    const ImVec2 b((float)luaL_checknumber(L, 3), (float)luaL_checknumber(L, 4));
    const ImVec2 c((float)luaL_checknumber(L, 5), (float)luaL_checknumber(L, 6));

    if (inFrame()) list()->AddTriangle(a, b, c, colorU32(L, 7), 1.f);
    return 0;
}

int l_draw_triangle_filled(lua_State* L) {
    const ImVec2 a((float)luaL_checknumber(L, 1), (float)luaL_checknumber(L, 2));
    const ImVec2 b((float)luaL_checknumber(L, 3), (float)luaL_checknumber(L, 4));
    const ImVec2 c((float)luaL_checknumber(L, 5), (float)luaL_checknumber(L, 6));

    if (inFrame()) list()->AddTriangleFilled(a, b, c, colorU32(L, 7));
    return 0;
}

int l_draw_quad(lua_State* L) {
    const ImVec2 a((float)luaL_checknumber(L, 1), (float)luaL_checknumber(L, 2));
    const ImVec2 b((float)luaL_checknumber(L, 3), (float)luaL_checknumber(L, 4));
    const ImVec2 c((float)luaL_checknumber(L, 5), (float)luaL_checknumber(L, 6));
    const ImVec2 d((float)luaL_checknumber(L, 7), (float)luaL_checknumber(L, 8));

    if (inFrame()) list()->AddQuad(a, b, c, d, colorU32(L, 9), 1.f);
    return 0;
}

int l_draw_quad_filled(lua_State* L) {
    const ImVec2 a((float)luaL_checknumber(L, 1), (float)luaL_checknumber(L, 2));
    const ImVec2 b((float)luaL_checknumber(L, 3), (float)luaL_checknumber(L, 4));
    const ImVec2 c((float)luaL_checknumber(L, 5), (float)luaL_checknumber(L, 6));
    const ImVec2 d((float)luaL_checknumber(L, 7), (float)luaL_checknumber(L, 8));

    if (inFrame()) list()->AddQuadFilled(a, b, c, d, colorU32(L, 9));
    return 0;
}

int l_draw_ngon(lua_State* L) {
    const float x        = (float)luaL_checknumber(L, 1);
    const float y        = (float)luaL_checknumber(L, 2);
    const float r        = (float)luaL_checknumber(L, 3);
    const int   segments = (int)luaL_checkinteger(L, 4);

    if (segments < 3 || segments > 128)
        return luaL_error(L, "an ngon needs between 3 and 128 segments");

    if (inFrame()) list()->AddNgon(ImVec2(x, y), r, colorU32(L, 5), segments, 1.f);
    return 0;
}

int l_draw_ngon_filled(lua_State* L) {
    const float x        = (float)luaL_checknumber(L, 1);
    const float y        = (float)luaL_checknumber(L, 2);
    const float r        = (float)luaL_checknumber(L, 3);
    const int   segments = (int)luaL_checkinteger(L, 4);

    if (segments < 3 || segments > 128)
        return luaL_error(L, "an ngon needs between 3 and 128 segments");

    if (inFrame()) list()->AddNgonFilled(ImVec2(x, y), r, colorU32(L, 5), segments);
    return 0;
}

int l_draw_bezier(lua_State* L) {
    const ImVec2 a((float)luaL_checknumber(L, 1), (float)luaL_checknumber(L, 2));
    const ImVec2 b((float)luaL_checknumber(L, 3), (float)luaL_checknumber(L, 4));
    const ImVec2 c((float)luaL_checknumber(L, 5), (float)luaL_checknumber(L, 6));
    const ImVec2 d((float)luaL_checknumber(L, 7), (float)luaL_checknumber(L, 8));

    if (inFrame())
        list()->AddBezierCubic(a, b, c, d, colorU32(L, 9),
                               (float)luaL_optnumber(L, 13, 1.0), 0);
    return 0;
}

int l_draw_polyline(lua_State* L) {
    std::vector<ImVec2> points;
    readPoints(L, 1, points);

    const ImU32 color     = colorU32(L, 2);
    const float thickness = (float)luaL_optnumber(L, 6, 1.0);
    const bool  closed    = lua_toboolean(L, 7) != 0;

    if (inFrame())
        list()->AddPolyline(points.data(), (int)points.size(), color,
                            closed ? ImDrawFlags_Closed : ImDrawFlags_None, thickness);
    return 0;
}

int l_draw_poly_filled(lua_State* L) {
    std::vector<ImVec2> points;
    readPoints(L, 1, points);

    if (inFrame())
        list()->AddConvexPolyFilled(points.data(), (int)points.size(), colorU32(L, 2));
    return 0;
}

int l_draw_text_scaled(lua_State* L) {
    const float x    = (float)luaL_checknumber(L, 1);
    const float y    = (float)luaL_checknumber(L, 2);
    const float size = (float)luaL_checknumber(L, 3);
    const char* text = luaL_checkstring(L, 4);

    if (size < 1.f || size > 256.f)
        return luaL_error(L, "text size must be between 1 and 256");

    if (inFrame())
        list()->AddText(ImGui::GetFont(), size, ImVec2(x, y), colorU32(L, 5), text);
    return 0;
}

int l_draw_push_clip(lua_State* L) {
    const float x = (float)luaL_checknumber(L, 1);
    const float y = (float)luaL_checknumber(L, 2);
    const float w = (float)luaL_checknumber(L, 3);
    const float h = (float)luaL_checknumber(L, 4);

    if (!inFrame()) return 0;

    list()->PushClipRect(ImVec2(x, y), ImVec2(x + w, y + h), true);
    pushScope(L, Scope::Clip, true);
    return 0;
}

int l_draw_pop_clip(lua_State* L) {
    if (popScope(L, Scope::Clip, "pop_clip")) list()->PopClipRect();
    return 0;
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

void closeImguiScopes() {
    if (g_scopes.empty()) return;

    if (!inFrame()) {
        g_scopes.clear();
        return;
    }

    while (!g_scopes.empty()) {
        const ScopeEntry entry = g_scopes.back();
        g_scopes.pop_back();
        closeScope(entry);
    }
}

void registerImguiApi(lua_State* L) {
    static const luaL_Reg kImgui[] = {

        { "begin_window",      l_begin_window      },
        { "end_window",        l_end_window        },
        { "begin_child",       l_begin_child       },
        { "end_child",         l_end_child         },
        { "begin_group",       l_begin_group       },
        { "end_group",         l_end_group         },
        { "set_next_pos",      l_set_next_pos      },
        { "set_next_size",     l_set_next_size     },
        { "set_next_bg_alpha", l_set_next_bg_alpha },
        { "window_pos",        l_window_pos        },
        { "window_size",       l_window_size       },
        { "content_avail",     l_content_avail     },
        { "cursor_pos",        l_cursor_pos        },
        { "set_cursor_pos",    l_set_cursor_pos    },
        { "cursor_screen_pos", l_cursor_screen_pos },
        { "same_line",         l_same_line         },
        { "spacing",           l_spacing           },
        { "new_line",          l_new_line          },
        { "separator",         l_separator         },
        { "indent",            l_indent            },
        { "unindent",          l_unindent          },
        { "dummy",             l_dummy             },
        { "set_next_width",    l_set_next_width    },
        { "push_item_width",   l_push_item_width   },
        { "pop_item_width",    l_pop_item_width    },
        { "push_id",           l_push_id           },
        { "pop_id",            l_pop_id            },
        { "begin_disabled",    l_begin_disabled    },
        { "end_disabled",      l_end_disabled      },

        { "push_style_color",  l_push_style_color  },
        { "pop_style_color",   l_pop_style_color   },
        { "push_style_var",    l_push_style_var    },
        { "pop_style_var",     l_pop_style_var     },
        { "style_color",       l_style_color       },

        { "text",              l_text              },
        { "text_colored",      l_text_colored      },
        { "text_disabled",     l_text_disabled     },
        { "text_wrapped",      l_text_wrapped      },
        { "bullet_text",       l_bullet_text       },
        { "label_text",        l_label_text        },
        { "text_size",         l_text_size         },

        { "button",            l_button            },
        { "small_button",      l_small_button      },
        { "invisible_button",  l_invisible_button  },
        { "checkbox",          l_checkbox          },
        { "radio",             l_radio             },
        { "slider",            l_slider            },
        { "slider_int",        l_slider_int        },
        { "drag",              l_drag              },
        { "drag_int",          l_drag_int          },
        { "input_text",        l_input_text        },
        { "input_multiline",   l_input_text_multiline },
        { "input_number",      l_input_number      },
        { "combo",             l_combo             },
        { "listbox",           l_listbox           },
        { "color_edit",        l_color_edit        },
        { "progress_bar",      l_progress_bar      },
        { "selectable",        l_selectable        },
        { "collapsing_header", l_collapsing_header },
        { "tree_node",         l_tree_node         },
        { "tree_pop",          l_tree_pop          },
        { "begin_tab_bar",     l_begin_tab_bar     },
        { "end_tab_bar",       l_end_tab_bar       },
        { "begin_tab_item",    l_begin_tab_item    },
        { "end_tab_item",      l_end_tab_item      },
        { "begin_combo",       l_begin_combo       },
        { "end_combo",         l_end_combo         },

        { "begin_table",       l_begin_table       },
        { "end_table",         l_end_table         },
        { "table_column",      l_table_setup_column},
        { "table_headers",     l_table_headers_row },
        { "table_next_row",    l_table_next_row    },
        { "table_next_column", l_table_next_column },
        { "table_set_column",  l_table_set_column  },

        { "open_popup",        l_open_popup        },
        { "begin_popup",       l_begin_popup       },
        { "begin_popup_modal", l_begin_popup_modal },
        { "end_popup",         l_end_popup         },
        { "close_popup",       l_close_popup       },
        { "set_tooltip",       l_set_tooltip       },

        { "plot_lines",        l_plot_lines        },
        { "plot_histogram",    l_plot_histogram    },
        { "is_item_hovered",   l_is_item_hovered   },
        { "is_item_clicked",   l_is_item_clicked   },
        { "is_item_active",    l_is_item_active    },
        { "is_window_hovered", l_is_window_hovered },
        { "is_window_focused", l_is_window_focused },
        { "frame_height",      l_frame_height      },
        { "font_size",         l_font_size         },
        { nullptr,             nullptr             },
    };

    lua_newtable(L);
    for (const luaL_Reg* f = kImgui; f->name; ++f) {
        lua_pushcfunction(L, f->func);
        lua_setfield(L, -2, f->name);
    }
    lua_setglobal(L, "imgui");

    static const luaL_Reg kDrawExtras[] = {
        { "circle_filled",       l_draw_circle_filled       },
        { "rect_rounded",        l_draw_rect_rounded        },
        { "rect_rounded_filled", l_draw_rect_rounded_filled },
        { "rect_gradient",       l_draw_rect_gradient       },
        { "triangle",            l_draw_triangle            },
        { "triangle_filled",     l_draw_triangle_filled     },
        { "quad",                l_draw_quad                },
        { "quad_filled",         l_draw_quad_filled         },
        { "ngon",                l_draw_ngon                },
        { "ngon_filled",         l_draw_ngon_filled         },
        { "bezier",              l_draw_bezier              },
        { "polyline",            l_draw_polyline            },
        { "poly_filled",         l_draw_poly_filled         },
        { "text_scaled",         l_draw_text_scaled         },
        { "push_clip",           l_draw_push_clip           },
        { "pop_clip",            l_draw_pop_clip            },
        { nullptr,               nullptr                     },
    };
    addFunctions(L, "draw", kDrawExtras);
}

}
