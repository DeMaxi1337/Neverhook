#pragma once

#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif

#include "framework_widgets.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "hashes.hpp"
#include "vars.h"
#include "blur.hpp"
#include "../lua/LuaEngine.hpp"

#include <Geode/Geode.hpp>
#include <Geode/utils/web.hpp>
#include <cstring>
#include <string>

using namespace ImGui;

inline const char* kNhDocsUrl      = "https://demaxi.gitbook.io/neverhook-lua-api";
inline const char* kNhDocsStartUrl = "https://demaxi.gitbook.io/neverhook-lua-api/getting-started";
inline const char* kNhDocsNodesUrl = "https://demaxi.gitbook.io/neverhook-lua-api/api/nodes";
inline const char* kNhDocsHooksUrl = "https://demaxi.gitbook.io/neverhook-lua-api/api/hooks";

inline bool  g_scriptsListOpen = true;
inline bool  g_scriptsConsole  = false;
inline float g_scriptsConsoleAnim = 0.f;
inline bool  g_consoleSelectMode = false;
inline int   g_consoleSelected   = -1;
inline std::string g_consoleText;
inline char  g_scriptSearch[64] = "";
inline char  g_scriptNewName[64] = "";
inline bool  g_scriptsScanned = false;

inline float g_scriptScroll       = 0.f;
inline float g_scriptScrollTarget = 0.f;

inline void nh_script_row(nh::lua::Script* script, int index) {
    auto* window = GetCurrentWindow();
    auto* draw   = window->DrawList;

    const float width  = GetContentRegionAvail().x;
    const float height = 48.f;
    const ImVec2 pos   = window->DC.CursorPos;

    PushID(index);

    const ImRect bb(pos, pos + ImVec2(width, height));
    ItemSize(bb, 0.f);
    ItemAdd(bb, window->GetID("##row"));

    const bool hovered = IsMouseHoveringRect(bb.Min, bb.Max);

    draw->AddRectFilled(bb.Min, bb.Max,
        hovered ? gui.button_hovered.to_im_color() : gui.button_bg.to_im_color(), 5.f);
    draw->AddRect(bb.Min, bb.Max, gui.border.to_im_color(2.5f), 5.f);

    const float centerY = bb.GetCenter().y;

    draw->AddText(ImVec2(bb.Min.x + 14.f, centerY - 17.f),
                  GetColorU32(ImGuiCol_Text), script->id().c_str());

    const std::string sub = "Modified: " + script->modified();
    draw->AddText(ImVec2(bb.Min.x + 14.f, centerY + 1.f),
                  gui.text_disabled.to_im_color(), sub.c_str());

    const ImVec2 btnSize(86.f, 26.f);
    SetCursorScreenPos(ImVec2(bb.Max.x - btnSize.x - 10.f, centerY - btnSize.y / 2.f));

    if (script->running()) {
        if (gui.button(ICON_FA_STOP " Stop", btnSize))
            script->stop();
    }
    else {
        PushStyleColor(ImGuiCol_Button,        gui.accent_color.to_vec4(0.85f));
        PushStyleColor(ImGuiCol_ButtonHovered, gui.accent_color.to_vec4(1.0f));
        PushStyleColor(ImGuiCol_ButtonActive,  gui.accent_color.to_vec4(0.7f));
        PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.5f, 0.5f));
        if (Button(ICON_FA_PLAY " Load", btnSize))
            script->start();
        PopStyleVar();
        PopStyleColor(3);
    }

    SetCursorScreenPos(ImVec2(bb.Max.x - btnSize.x - 44.f, centerY - 13.f));
    if (gui.button("...", ImVec2(28.f, 26.f)))
        OpenPopup("##rowmenu");

    PushStyleVar(ImGuiStyleVar_WindowPadding,      ImVec2(10.f, 10.f));
    PushStyleVar(ImGuiStyleVar_ItemSpacing,        ImVec2(8.f, 6.f));
    PushStyleVar(ImGuiStyleVar_SelectableTextAlign, ImVec2(0.f, 0.5f));

    if (BeginPopup("##rowmenu")) {
        const ImVec2 itemSize(184.f, 26.f);

        if (Selectable("  " ICON_FA_SYNC "   Reload", false, 0, itemSize)) {
            script->refreshTimestamp();
            if (script->running()) script->start();
        }

        if (Selectable("  " ICON_FA_HOME "   Open scripts folder", false, 0, itemSize))
            nh::lua::Manager::get().openFolder();

        Spacing();
        Separator();
        Spacing();

        PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 0.42f, 0.42f, 1.f));
        if (Selectable("  " ICON_FA_STOP "   Delete file", false, 0, itemSize)) {
            script->stop();

            std::error_code ec;
            std::filesystem::remove(script->path(), ec);

            nh::lua::Manager::get().log(
                ec ? script->fileName() + ": could not delete"
                   : script->fileName() + " deleted",
                (bool)ec);

            nh::lua::Manager::get().refresh();
        }
        PopStyleColor();

        EndPopup();
    }

    PopStyleVar(3);

    if (!script->running() && !script->lastError().empty()) {
        SetCursorScreenPos(ImVec2(bb.Min.x + 14.f, bb.Max.y + 2.f));
        PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 0.42f, 0.42f, 1.f));
        TextWrapped("%s", script->lastError().c_str());
        PopStyleColor();
    }
    else {
        SetCursorScreenPos(ImVec2(bb.Min.x, bb.Max.y));
    }

    Dummy(ImVec2(0.f, 6.f));
    PopID();
}

inline std::string ConsoleJoin() {
    std::string out;
    for (const auto& line : nh::lua::Manager::get().console()) {
        out += line.text;
        out += '\n';
    }
    return out;
}

inline void DrawScriptConsoleWindow() {
    g_scriptsConsoleAnim = fi_lerp(g_scriptsConsoleAnim, g_scriptsConsole ? 1.f : 0.f, 0.30f);
    if (g_scriptsConsoleAnim < 0.004f)
        return;

    auto& mgr = nh::lua::Manager::get();
    ImGuiIO& io = GetIO();

    SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.75f),
                     ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    SetNextWindowSize(ImVec2(480.f, 240.f), ImGuiCond_Appearing);
    SetNextWindowSizeConstraints(ImVec2(320.f, 140.f), ImVec2(FLT_MAX, FLT_MAX));

    PushStyleVar(ImGuiStyleVar_Alpha, g_scriptsConsoleAnim);
    PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));
    SetNextWindowBgAlpha(g_scriptsConsoleAnim);

    Begin("##NeverhookScriptConsole", nullptr,
          ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings);
    {
        ImGuiWindow* window = GetCurrentWindow();
        ImDrawList*  draw   = window->DrawList;
        const ImVec2 pos    = window->Pos;
        const ImVec2 size   = window->Size;

        const bool  consoleBlur = Vars::guiBlur && nh::blur::available();
        const float consoleMul  = consoleBlur ? 0.78f : 1.f;

        if (consoleBlur)
            nh::blur::submit(GetBackgroundDrawList(), pos, pos + size, 6.f,
                             Vars::guiBlurStrength, g_scriptsConsoleAnim);

        draw->AddRectFilled(pos, pos + size,
            ImColor(0.019f, 0.035f, 0.062f, g_scriptsConsoleAnim * consoleMul), 6.f);
        draw->AddRect(pos, pos + size, gui.border.to_im_color(), 6.f);

        const float barH = 34.f;
        draw->AddText(ImVec2(pos.x + 12.f, pos.y + 9.f),
                      gui.text_disabled.to_im_color(), ICON_FA_TERMINAL);
        draw->AddText(ImVec2(pos.x + 34.f, pos.y + 9.f),
                      GetColorU32(ImGuiCol_Text), "Script console");
        draw->AddLine(ImVec2(pos.x, pos.y + barH), ImVec2(pos.x + size.x, pos.y + barH),
                      gui.border.to_im_color(2.5f));

        SetCursorPos(ImVec2(size.x - 208.f, 5.f));
        if (gui.button(g_consoleSelectMode ? "Lines" : "Select", ImVec2(54.f, 24.f))) {
            g_consoleSelectMode = !g_consoleSelectMode;
            g_consoleSelected   = -1;
        }

        SetCursorPos(ImVec2(size.x - 148.f, 5.f));
        if (gui.button("Copy", ImVec2(54.f, 24.f)))
            SetClipboardText(ConsoleJoin().c_str());

        SetCursorPos(ImVec2(size.x - 88.f, 5.f));
        if (gui.button("Clear", ImVec2(50.f, 24.f))) {
            mgr.clearConsole();
            g_consoleSelected = -1;
            g_consoleText.clear();
        }

        SetCursorPos(ImVec2(size.x - 32.f, 5.f));
        if (gui.button("X", ImVec2(24.f, 24.f)))
            g_scriptsConsole = false;

        SetCursorPos(ImVec2(8.f, barH + 6.f));
        BeginChild("##consolebody", ImVec2(size.x - 16.f, size.y - barH - 14.f));

        if (mgr.console().empty() && !g_consoleSelectMode)
            TextDisabled("Nothing logged yet.");

        if (g_consoleSelectMode) {
            g_consoleText = ConsoleJoin();
            if (g_consoleText.empty())
                g_consoleText = "Nothing logged yet.";

            InputTextMultiline("##consoleraw", g_consoleText.data(), g_consoleText.size() + 1,
                               ImVec2(-1.f, -1.f), ImGuiInputTextFlags_ReadOnly);
        }
        else {
            int index = 0;
            for (const auto& line : mgr.console()) {
                PushID(index);

                if (line.error) PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 0.42f, 0.42f, 1.f));
                if (Selectable(line.text.c_str(), g_consoleSelected == index))
                    g_consoleSelected = index;
                if (line.error) PopStyleColor();

                if (BeginPopupContextItem("##consolectx")) {
                    g_consoleSelected = index;
                    if (Selectable("Copy line"))
                        SetClipboardText(line.text.c_str());
                    if (Selectable("Copy all"))
                        SetClipboardText(ConsoleJoin().c_str());
                    if (Selectable("Clear")) {
                        mgr.clearConsole();
                        g_consoleSelected = -1;
                        EndPopup();
                        PopID();
                        break;
                    }
                    EndPopup();
                }

                PopID();
                ++index;
            }

            if (g_consoleSelected >= 0 && g_consoleSelected < (int)mgr.console().size() &&
                io.KeyCtrl && IsKeyPressed(ImGuiKey_C, false))
                SetClipboardText(mgr.console()[g_consoleSelected].text.c_str());

            if (g_consoleSelected < 0 && GetScrollY() >= GetScrollMaxY() - 4.f)
                SetScrollHereY(1.f);
        }

        EndChild();
    }
    End();

    PopStyleVar(2);
}

inline void DrawScriptsTab() {
    auto& mgr = nh::lua::Manager::get();

    if (!g_scriptsScanned) {
        mgr.refresh();
        g_scriptsScanned = true;
    }

    Dummy(ImVec2(0.f, 3.f));

    const float avail   = GetContentRegionAvail().x;
    const float spacing = 6.f;
    const float toolH   = 26.f;
    const float toolsW  = 30.f + 30.f + 110.f + spacing * 3.f;

    PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.f, 4.f));

    PushItemWidth(avail - toolsW);
    InputTextWithHint("##scriptsearch", ICON_FA_SEARCH "  Search", g_scriptSearch,
                      IM_ARRAYSIZE(g_scriptSearch));
    PopItemWidth();

    SameLine(0.f, spacing);
    if (gui.button(ICON_FA_SYNC, ImVec2(30.f, toolH)))
        mgr.refresh();

    SameLine(0.f, spacing);
    if (gui.button(ICON_FA_TERMINAL, ImVec2(30.f, toolH)))
        g_scriptsConsole = !g_scriptsConsole;

    SameLine(0.f, spacing);
    PushStyleColor(ImGuiCol_Button,        gui.accent_color.to_vec4(0.85f));
    PushStyleColor(ImGuiCol_ButtonHovered, gui.accent_color.to_vec4(1.0f));
    PushStyleColor(ImGuiCol_ButtonActive,  gui.accent_color.to_vec4(0.7f));
    PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.5f, 0.5f));
    if (Button(ICON_FA_PLUS " Create", ImVec2(110.f, toolH)))
        OpenPopup("##createscript");
    PopStyleVar();
    PopStyleColor(3);

    PopStyleVar();

    PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10.f, 10.f));
    if (BeginPopup("##createscript")) {
        TextDisabled("New script");
        Spacing();

        PushItemWidth(200.f);
        InputTextWithHint("##newname", "name", g_scriptNewName, IM_ARRAYSIZE(g_scriptNewName));
        PopItemWidth();

        Spacing();
        if (gui.button("Create", ImVec2(200.f, 26.f))) {
            if (mgr.createScript(g_scriptNewName))
                g_scriptNewName[0] = '\0';
            CloseCurrentPopup();
        }
        EndPopup();
    }
    PopStyleVar();

    Dummy(ImVec2(0.f, 10.f));

    {
        const std::string header = std::string(g_scriptsListOpen ? ICON_FA_ANGLE_DOWN
                                                                 : ICON_FA_ANGLE_RIGHT)
                                 + "  My Items";
        PushStyleColor(ImGuiCol_Text, gui.text_disabled.to_vec4());
        if (Selectable(header.c_str(), false, 0, ImVec2(0.f, 20.f)))
            g_scriptsListOpen = !g_scriptsListOpen;
        PopStyleColor();
    }

    const float footerH = 30.f;

    BeginChild("##scriptlist",
               ImVec2(0.f, GetContentRegionAvail().y - footerH - GetStyle().ItemSpacing.y * 2.f),
               false, ImGuiWindowFlags_NoScrollWithMouse);

    if (g_scriptsListOpen) {
        int shown = 0;
        int index = 0;

        for (auto& script : mgr.scripts()) {
            if (!script) continue;

            if (g_scriptSearch[0] != '\0' &&
                script->id().find(g_scriptSearch) == std::string::npos) {
                ++index;
                continue;
            }

            nh_script_row(script.get(), index++);
            ++shown;
        }

        if (shown == 0) {
            Spacing();
            TextDisabled("  No scripts yet.");
            TextDisabled("  Press Create, or drop a .lua file into the scripts folder.");
        }
    }

    {
        ImGuiIO& io = GetIO();
        const float maxScroll = GetScrollMaxY();

        if (IsWindowHovered(ImGuiHoveredFlags_ChildWindows) && io.MouseWheel != 0.f)
            g_scriptScrollTarget -= io.MouseWheel * 48.f;

        g_scriptScrollTarget = ImClamp(g_scriptScrollTarget, 0.f, maxScroll);
        g_scriptScroll       = fi_lerp(g_scriptScroll, g_scriptScrollTarget, 0.22f);

        SetScrollY(ImClamp(g_scriptScroll, 0.f, maxScroll));
    }

    EndChild();

    {
        ImDrawList* list = GetWindowDrawList();
        const float padX = GetStyle().WindowPadding.x;
        const float x0   = GetWindowPos().x + padX;
        const float x1   = GetWindowPos().x + GetWindowWidth() - padX;
        const float y    = GetCursorScreenPos().y;

        list->AddLine(ImVec2(x0, y), ImVec2(x1, y), gui.border.to_im_color(2.5f));
    }

    Dummy(ImVec2(0.f, 8.f));

    const float linkW  = 122.f;
    const float totalW = linkW * 2.f + spacing;

    SetCursorPosX((GetWindowWidth() - totalW) * 0.5f);

    PushStyleVar(ImGuiStyleVar_SelectableTextAlign, ImVec2(0.5f, 0.5f));
    PushStyleColor(ImGuiCol_Text, gui.text_disabled.to_vec4());

    if (Selectable("Documentation", false, 0, ImVec2(linkW, 0.f)))
        geode::utils::web::openLinkInBrowser(kNhDocsStartUrl);

    if (BeginPopupContextItem("##docsmenu")) {
        if (Selectable("Getting started"))
            geode::utils::web::openLinkInBrowser(kNhDocsStartUrl);
        if (Selectable("Nodes"))
            geode::utils::web::openLinkInBrowser(kNhDocsNodesUrl);
        if (Selectable("Hooks"))
            geode::utils::web::openLinkInBrowser(kNhDocsHooksUrl);
        if (Selectable("Full reference"))
            geode::utils::web::openLinkInBrowser(kNhDocsUrl);
        EndPopup();
    }

    SameLine();
    if (Selectable("Scripts folder", false, 0, ImVec2(linkW, 0.f)))
        mgr.openFolder();

    PopStyleColor();
    PopStyleVar();
}

#include <cmath>
#include <cstring>

inline void nh_styled_text(const nh::lua::TextStyle& style, const char* text) {
    if (!style.gradient && !style.rainbow) {
        TextWrapped("%s", text);
        return;
    }

    const ImVec2 origin = GetCursorScreenPos();
    ImDrawList*  list   = GetWindowDrawList();

    const float speed = style.speed <= 0.f ? 1.f : style.speed;
    const float time  = (float)GetTime() * speed;

    const float phase = style.animated ? (float)std::fmod(time * 0.25f, 1.f) : 0.f;

    const std::size_t count = std::strlen(text);

    float cursor = origin.x;
    for (std::size_t i = 0; i < count; ++i) {
        const char glyph[2] = { text[i], '\0' };

        float t = count > 1 ? (float)i / (float)(count - 1) : 0.f;
        t += phase;
        if (t > 1.f) t -= 1.f;

        ImU32 color;
        if (style.rainbow) {
            float hue = t * 0.6f + time * 0.15f;
            hue = hue - (float)(int)hue;
            color = (ImU32)ImColor::HSV(hue, 0.75f, 1.f, style.colorA[3]);
        }
        else {

            const float k = style.animated ? (t < 0.5f ? t * 2.f : (1.f - t) * 2.f) : t;

            color = GetColorU32(ImVec4(
                style.colorA[0] + (style.colorB[0] - style.colorA[0]) * k,
                style.colorA[1] + (style.colorB[1] - style.colorA[1]) * k,
                style.colorA[2] + (style.colorB[2] - style.colorA[2]) * k,
                style.colorA[3] + (style.colorB[3] - style.colorA[3]) * k));
        }

        list->AddText(ImVec2(cursor, origin.y), color, glyph, glyph + 1);
        cursor += CalcTextSize(glyph).x;
    }

    Dummy(ImVec2(cursor - origin.x, GetTextLineHeight()));
}

inline void DrawScriptMenuTab(nh::lua::MenuTab& tab) {
    gui.group_box(tab.title.c_str(), ImVec2(GetWindowWidth(), GetWindowHeight()));

    if (tab.widgets.empty())
        TextDisabled("This script did not add any controls.");

    int index  = 0;
    int indent = 0;

    for (auto& w : tab.widgets) {
        PushID(index++);

        const bool tinted = w.style.colored && !w.style.gradient && !w.style.rainbow;
        if (tinted)
            PushStyleColor(ImGuiCol_Text, ImVec4(w.style.colorA[0], w.style.colorA[1],
                                                 w.style.colorA[2], w.style.colorA[3]));

        switch (w.type) {
        case nh::lua::WidgetType::Toggle: {
            const bool before = w.boolValue();
            gui.toggle(w.label.c_str(), w.boolPtr());
            if (w.boolValue() != before)
                nh::lua::runWidgetChanged(tab.owner, w.changedRef);
            break;
        }

        case nh::lua::WidgetType::Slider: {
            const float before = w.fValue;
            gui.slider_float(w.label.c_str(), &w.fValue, w.minVal, w.maxVal);
            if (w.fValue != before)
                nh::lua::runWidgetChanged(tab.owner, w.changedRef);
            break;
        }

        case nh::lua::WidgetType::SliderInt: {
            const int before = w.iValue;
            gui.slider_int(w.label.c_str(), &w.iValue, (int)w.minVal, (int)w.maxVal);
            if (w.iValue != before)
                nh::lua::runWidgetChanged(tab.owner, w.changedRef);
            break;
        }

        case nh::lua::WidgetType::Combo: {
            std::vector<const char*> items;
            items.reserve(w.items.size());
            for (auto& s : w.items) items.push_back(s.c_str());

            const int before = w.iValue;
            Combo(w.label.c_str(), &w.iValue, items.data(), (int)items.size());
            if (w.iValue != before)
                nh::lua::runWidgetChanged(tab.owner, w.changedRef);
            break;
        }

        case nh::lua::WidgetType::Button:
            if (gui.button(w.label.c_str(), ImVec2(-1.f, 0.f)))
                nh::lua::runWidgetCallback(tab.owner, w.fnRef);
            break;

        case nh::lua::WidgetType::InputText: {
            char buffer[512];
            std::strncpy(buffer, w.sValue.c_str(), sizeof(buffer) - 1);
            buffer[sizeof(buffer) - 1] = '\0';

            SetNextItemWidth(-1.f);
            ImGui::InputTextWithHint("##input", w.label.c_str(), buffer, sizeof(buffer));

            if (std::strcmp(buffer, w.sValue.c_str()) != 0) {
                w.sValue = buffer;
                nh::lua::runWidgetChanged(tab.owner, w.changedRef);
            }
            break;
        }

        case nh::lua::WidgetType::ColorPicker: {
            float before[4] = { w.color[0], w.color[1], w.color[2], w.color[3] };

            ImGui::ColorEdit4(w.label.c_str(), w.color,
                              ImGuiColorEditFlags_AlphaBar |
                              ImGuiColorEditFlags_NoInputs);

            if (std::memcmp(before, w.color, sizeof(before)) != 0)
                nh::lua::runWidgetChanged(tab.owner, w.changedRef);
            break;
        }

        case nh::lua::WidgetType::GroupBegin:
            Spacing();
            if (w.style.colored) nh_styled_text(w.style, w.label.c_str());
            else                 TextDisabled("%s", w.label.c_str());
            Separator();
            Indent(10.f);
            ++indent;
            break;

        case nh::lua::WidgetType::GroupEnd:
            if (indent > 0) { Unindent(10.f); --indent; }
            Spacing();
            break;

        case nh::lua::WidgetType::Label:
            if (w.style.gradient || w.style.rainbow) nh_styled_text(w.style, w.label.c_str());
            else                                     TextWrapped("%s", w.label.c_str());
            break;

        case nh::lua::WidgetType::Separator:
            Separator();
            break;
        }

        if (tinted) PopStyleColor();

        PopID();
    }

    while (indent-- > 0) Unindent(10.f);

    gui.end_group_box();
}
