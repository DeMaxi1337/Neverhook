#pragma once

#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif

#include "framework_widgets.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "hashes.hpp"
#include "vars.h"

#include <unordered_map>
#include <string>
#include <vector>
#include <cstdio>
#include <ctime>
#include <cmath>
#include <chrono>

using namespace ImGui;

#if defined(ICON_FA_COG)
static const char* WM_GEAR = ICON_FA_COG;
#elif defined(ICON_FA_GEAR)
static const char* WM_GEAR = ICON_FA_GEAR;
#else
static const char* WM_GEAR = "*";
#endif

static bool wm_combo(const char* label, int* v, const char* const* items, int count)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    const ImGuiID id = window->GetID(label);
    const ImVec2  pos = window->DC.CursorPos;
    ImDrawList* draw = window->DrawList;

    const ImVec2 label_size = CalcTextSize(label, nullptr, true);
    const float  row_w = GetContentRegionAvail().x;
    const float  box_w = 130.f;
    const float  row_h = ImMax(label_size.y, 22.f);

    ImRect bb(pos, ImVec2(pos.x + row_w, pos.y + row_h));
    ItemSize(bb, GetStyle().FramePadding.y);
    if (!ItemAdd(bb, id))
        return false;

    const ImVec2 box_min(bb.Max.x - box_w, bb.GetCenter().y - 11.f);
    const ImVec2 box_max(bb.Max.x, bb.GetCenter().y + 11.f);

    bool hovered, held;
    const bool pressed = ButtonBehavior(ImRect(box_min, box_max), id, &hovered, &held);

    draw->AddText(ImVec2(pos.x, bb.GetCenter().y - label_size.y * 0.5f),
        GetColorU32(ImGuiCol_Text), label);

    draw->AddRectFilled(box_min, box_max,
        (hovered ? gui.button_active : gui.button_bg).to_im_color(), 4.f);
    draw->AddRect(box_min, box_max, gui.border.to_im_color(2.5f), 4.f);

    const char* cur = (*v >= 0 && *v < count) ? items[*v] : "";
    draw->AddText(ImVec2(box_min.x + 8.f, bb.GetCenter().y - label_size.y * 0.5f),
        GetColorU32(ImGuiCol_Text), cur);

    const ImVec2 cc(box_max.x - 12.f, bb.GetCenter().y - 1.f);
    draw->AddTriangleFilled(ImVec2(cc.x - 4.f, cc.y - 2.f), ImVec2(cc.x + 4.f, cc.y - 2.f),
        ImVec2(cc.x, cc.y + 3.f), GetColorU32(ImGuiCol_TextDisabled));

    char popup_id[48];
    ImFormatString(popup_id, IM_ARRAYSIZE(popup_id), "##wmc_%u", id);
    if (pressed)
        OpenPopup(popup_id);

    bool changed = false;
    SetNextWindowPos(ImVec2(box_min.x, box_max.y + 3.f));
    SetNextWindowSize(ImVec2(box_w, 0.f));
    PushStyleColor(ImGuiCol_PopupBg,       (ImU32)ImColor(0.020f, 0.035f, 0.062f, 0.98f));
    PushStyleColor(ImGuiCol_Border,        (ImU32)ImColor(1.f, 1.f, 1.f, 0.06f));
    PushStyleColor(ImGuiCol_Text,          (ImU32)gui.text.to_im_color());
    PushStyleColor(ImGuiCol_Header,        (ImU32)gui.accent_color.to_im_color(0.35f));
    PushStyleColor(ImGuiCol_HeaderHovered, (ImU32)gui.button_active.to_im_color());
    PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(6.f, 6.f));
    PushStyleVar(ImGuiStyleVar_WindowRounding, 4.f);
    if (BeginPopup(popup_id))
    {
        for (int i = 0; i < count; ++i)
            if (Selectable(items[i], i == *v)) { *v = i; changed = true; }
        EndPopup();
    }
    PopStyleVar(2);
    PopStyleColor(5);
    return changed;
}

static bool wm_row(const char* label, bool* v, bool has_gear)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    const ImGuiID id = window->GetID(label);
    ImDrawList* draw = window->DrawList;
    const ImVec2  pos = window->DC.CursorPos;

    const ImVec2 label_size = CalcTextSize(label, nullptr, true);
    const float  pill_w = 30.f, pill_h = 16.f;
    const float  row_w = GetContentRegionAvail().x;
    const float  row_h = ImMax(label_size.y, pill_h);

    ImRect bb(pos, ImVec2(pos.x + row_w, pos.y + row_h));
    ItemSize(bb, GetStyle().FramePadding.y);
    if (!ItemAdd(bb, id))
        return false;

    const ImVec2 pill_max(bb.Max.x, bb.GetCenter().y + pill_h * 0.5f);
    const ImVec2 pill_min(pill_max.x - pill_w, bb.GetCenter().y - pill_h * 0.5f);

    const float  gear_sz = 16.f;
    const ImRect gear_bb(
        ImVec2(pill_min.x - gear_sz - 10.f, bb.GetCenter().y - gear_sz * 0.5f),
        ImVec2(pill_min.x - 10.f,           bb.GetCenter().y + gear_sz * 0.5f));

    const bool over_gear = has_gear && IsMouseHoveringRect(gear_bb.Min, gear_bb.Max);

    bool hovered, held;
    const bool pressed = ButtonBehavior(bb, id, &hovered, &held);
    bool gear_pressed = false;
    if (pressed)
    {
        if (over_gear) gear_pressed = true;
        else           *v = !*v;
    }

    static std::unordered_map<ImGuiID, float> anims;
    auto it = anims.find(id);
    if (it == anims.end())
        it = anims.insert({ id, *v ? 1.f : 0.f }).first;
    it->second = fi_lerp(it->second, *v ? 1.f : 0.f, 0.18f);
    const float a = it->second;

    const ImVec4 off_v = ImColor(0.10f, 0.13f, 0.24f, gui.m_fade).Value;
    const ImVec4 on_v  = gui.accent_color.to_im_color().Value;
    draw->AddRectFilled(pill_min, pill_max, ImColor(ImLerp(off_v, on_v, a)), pill_h * 0.5f);

    const float knob_r = pill_h * 0.5f - 2.f;
    const float knob_x = ImLerp(pill_min.x + knob_r + 2.f, pill_max.x - knob_r - 2.f, a);
    draw->AddCircleFilled(ImVec2(knob_x, bb.GetCenter().y), knob_r,
        ImColor(1.f, 1.f, 1.f, gui.m_fade));

    if (has_gear)
    {
        const ImU32 gcol = (over_gear ? gui.text : gui.text_disabled).to_im_color();
        draw->AddText(ImVec2(gear_bb.Min.x, bb.GetCenter().y - label_size.y * 0.5f), gcol, WM_GEAR);
    }

    draw->AddText(ImVec2(pos.x, bb.GetCenter().y - label_size.y * 0.5f),
        GetColorU32(ImGuiCol_Text), label);

    return gear_pressed;
}

static void wm_name_color_popup()
{
    SetNextWindowSize(ImVec2(220.f, 0.f));
    PushStyleColor(ImGuiCol_PopupBg,        (ImU32)ImColor(0.020f, 0.035f, 0.062f, 0.98f));
    PushStyleColor(ImGuiCol_Border,         (ImU32)ImColor(1.f, 1.f, 1.f, 0.06f));
    PushStyleColor(ImGuiCol_Text,           (ImU32)gui.text.to_im_color());
    PushStyleColor(ImGuiCol_FrameBg,        (ImU32)gui.frame_inactive.to_im_color());
    PushStyleColor(ImGuiCol_FrameBgHovered, (ImU32)gui.frame_active.to_im_color());
    PushStyleColor(ImGuiCol_FrameBgActive,  (ImU32)gui.frame_active.to_im_color());
    PushStyleColor(ImGuiCol_Header,         (ImU32)gui.accent_color.to_im_color(0.35f));
    PushStyleColor(ImGuiCol_HeaderHovered,  (ImU32)gui.button_active.to_im_color());
    PushStyleVar(ImGuiStyleVar_WindowRounding, 4.f);
    if (BeginPopup("##wm_name_cfg"))
    {
        TextDisabled("Cheat Name Style");
        Separator();
        Combo("Color Mode", &Vars::wmNameColorMode, "Solid\0Animated Gradient\0");
        ColorEdit4("Color A", Vars::wmColorOne,
            ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
        if (Vars::wmNameColorMode == 1)
            ColorEdit4("Color B", Vars::wmColorTwo,
                ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
        EndPopup();
    }
    PopStyleVar(1);
    PopStyleColor(8);
}

inline void DrawWatermarkSettings()
{
    gui.toggle("Watermark", &Vars::watermark);

    if (!Vars::watermark)
        return;

    Spacing();

    static const char* styles[] = { "Version 1.5", "Onetap v3", "Skeet" };
    wm_combo("Style", &Vars::watermarkStyle, styles, IM_ARRAYSIZE(styles));

    static const char* line_modes[] = { "Static", "Gradient" };
    wm_combo("Line", &Vars::watermarkLine, line_modes, IM_ARRAYSIZE(line_modes));

    static const char* positions[] = { "Top Left", "Bottom Left", "Top Right", "Bottom Right", "Bottom Center" };
    wm_combo("Position", &Vars::watermarkPos, positions, IM_ARRAYSIZE(positions));

    gui.toggle("Glow",       &Vars::wmGlow);
    gui.toggle("Global Color", &Vars::wmGlobal);
    gui.toggle("Background", &Vars::wmBackground);

    Spacing();
    gui.group_title("Elements");
    Spacing();

    if (wm_row("Cheat Name", &Vars::wmShowName, true))
        OpenPopup("##wm_name_cfg");
    wm_name_color_popup();

    wm_row("FPS",       &Vars::wmShowFps,  false);
    wm_row("Clock",     &Vars::wmShowTime, false);
    wm_row("Noclip Accuracy", &Vars::wmShowNcAcc, false);
    wm_row("Noclip Deaths",   &Vars::wmShowNcDeaths, false);
    wm_row("Frame Counter",   &Vars::wmShowFrame, false);
}

#include "../hooks/FrameAdvanceState.hpp"

inline void DrawWatermark()
{
    if (!Vars::watermark)
        return;

    ImGuiIO& io = GetIO();
    ImDrawList* draw = GetForegroundDrawList();

    struct Seg { std::string txt; bool is_name; };
    std::vector<Seg> segs;

    if (Vars::wmShowName) segs.push_back({ "Neverhook", true });
    if (Vars::wmShowFps)
    {

        using clock = std::chrono::steady_clock;
        static clock::time_point s_last  = clock::now();
        static double s_window = 0.0;
        static int    s_frames = 0;
        static double s_shown  = 0.0;

        const clock::time_point now = clock::now();
        const double dt = std::chrono::duration<double>(now - s_last).count();
        s_last = now;
        if (dt > 0.0 && dt < 1.0) { s_window += dt; ++s_frames; }
        if (s_window >= 0.25) { s_shown = s_frames / s_window; s_window = 0.0; s_frames = 0; }

        char b[32];
        std::snprintf(b, sizeof(b), "%.0f fps", s_shown);
        segs.push_back({ b, false });
    }
    if (Vars::wmShowTime)
    {
        std::time_t t = std::time(nullptr);
        std::tm lt{};
        localtime_s(&lt, &t);
        char b[32];
        std::snprintf(b, sizeof(b), "%02d:%02d:%02d", lt.tm_hour, lt.tm_min, lt.tm_sec);
        segs.push_back({ b, false });
    }

    if (Vars::wmShowNcAcc && Vars::noclip)
    {
        char b[32];
        std::snprintf(b, sizeof(b), "%.2f%%", Vars::noclipAccuracy);
        segs.push_back({ b, false });
    }
    if (Vars::wmShowNcDeaths && Vars::noclip)
    {
        char b[32];
        std::snprintf(b, sizeof(b), "%d deaths", Vars::noclipDeaths);
        segs.push_back({ b, false });
    }

    if (Vars::wmShowFrame)
    {
        int fr = 0;
        if (auto gjbgl = GJBaseGameLayer::get()) fr = gjbgl->m_gameState.m_currentProgress / kProgressPerFrame;
        char b[32];
        std::snprintf(b, sizeof(b), "frame %d", fr);
        segs.push_back({ b, false });
    }

    if (segs.empty())
        return;

    const ImVec4 c1(Vars::wmColorOne[0], Vars::wmColorOne[1], Vars::wmColorOne[2], Vars::wmColorOne[3]);
    const ImVec4 c2(Vars::wmColorTwo[0], Vars::wmColorTwo[1], Vars::wmColorTwo[2], Vars::wmColorTwo[3]);

    const float gphase = (float)GetTime();

    const ImU32 accent_solid = Vars::wmGlobal
        ? (ImU32)gui.accent_color.to_im_color()
        : (ImU32)ImColor(c1);

    ImU32 name_col;
    if (Vars::wmNameColorMode == 1)
    {
        const float t = 0.5f + 0.5f * sinf(gphase * 2.f);
        name_col = ImColor(ImLerp(c1, c2, t));
    }
    else
        name_col = accent_solid;

    const bool line_gradient = (Vars::watermarkLine == 1);

    auto faded = [&](float alpha) -> ImU32
    {
        if (Vars::wmGlobal) return gui.accent_color.to_im_color(alpha);
        return (ImU32)ImColor(c1.x, c1.y, c1.z, alpha);
    };

    auto draw_line = [&](float x0, float x1, float y0, float thickness)
    {
        if (line_gradient)
        {
            const float k = 0.5f + 0.5f * sinf(gphase * 2.f);
            const ImU32 cl = ImColor(ImLerp(c1, c2, k));
            const ImU32 cr = ImColor(ImLerp(c2, c1, k));
            draw->AddRectFilledMultiColor(ImVec2(x0, y0), ImVec2(x1, y0 + thickness),
                cl, cr, cr, cl);
        }
        else
            draw->AddRectFilled(ImVec2(x0, y0), ImVec2(x1, y0 + thickness), accent_solid);
    };

    const int style = Vars::watermarkStyle;

    const char* sep = "  |  ";
    const ImVec2 sep_sz = CalcTextSize(sep);
    const float  text_h = CalcTextSize("A").y;

    const float pad_x   = 12.f;
    const float pad_y   = 6.f;
    const float top_pad = (style == 2) ?  4.f :  0.f;

    float text_w = 0.f;
    for (size_t i = 0; i < segs.size(); ++i)
    {
        text_w += CalcTextSize(segs[i].txt.c_str()).x;
        if (i + 1 < segs.size())
            text_w += sep_sz.x;
    }

    const ImVec2 box_sz(text_w + pad_x * 2.f, text_h + pad_y * 2.f + top_pad);
    const float wm_margin = 14.f;
    float wm_x, wm_y;
    switch (Vars::watermarkPos)
    {
    case 0: wm_x = wm_margin;                                wm_y = wm_margin; break;
    case 1: wm_x = wm_margin;                                wm_y = io.DisplaySize.y - box_sz.y - wm_margin; break;
    case 3: wm_x = io.DisplaySize.x - box_sz.x - wm_margin; wm_y = io.DisplaySize.y - box_sz.y - wm_margin; break;
    case 4: wm_x = (io.DisplaySize.x - box_sz.x) * 0.5f;    wm_y = io.DisplaySize.y - box_sz.y - wm_margin; break;
    case 2: default: wm_x = io.DisplaySize.x - box_sz.x - wm_margin; wm_y = wm_margin; break;
    }
    const ImVec2 box_pos(wm_x, wm_y);
    const ImVec2 box_end(box_pos.x + box_sz.x, box_pos.y + box_sz.y);

    const float rounding = (style == 0) ? 3.f : 0.f;

    if (style == 2)
    {
        if (Vars::wmBackground)
        {
            draw->AddRectFilled(box_pos, box_end, (ImU32)ImColor(20, 20, 20, 235));
            draw->AddRect(box_pos, box_end, (ImU32)ImColor(18, 18, 18, 255));
            draw->AddRect(ImVec2(box_pos.x + 1.f, box_pos.y + 1.f),
                ImVec2(box_end.x - 1.f, box_end.y - 1.f),
                (ImU32)ImColor(62, 62, 62, 255));
        }

        const float bx0 = box_pos.x + 3.f, bx1 = box_end.x - 3.f;
        const float by  = box_pos.y + 3.f;
        {
            const float mid = (bx0 + bx1) * 0.5f;
            for (int r = 0; r < 2; ++r)
            {
                const float yy = by + (float)r;
                const float f  = (r == 0) ? 1.f : 0.5f;
                const ImU32 blue   = (ImU32)ImColor(int(59*f),  int(175*f), int(222*f), 255);
                const ImU32 purple = (ImU32)ImColor(int(202*f), int(70*f),  int(205*f), 255);
                const ImU32 green  = (ImU32)ImColor(int(201*f), int(227*f), int(58*f),  255);
                draw->AddRectFilledMultiColor(ImVec2(bx0, yy), ImVec2(mid, yy + 1.f),
                    blue, purple, purple, blue);
                draw->AddRectFilledMultiColor(ImVec2(mid, yy), ImVec2(bx1, yy + 1.f),
                    purple, green, green, purple);
            }
        }

        const ImU32 col_text = (ImU32)gui.text.to_im_color();
        const float row_top  = by + 3.f;
        const ImU32 col_sep = (ImU32)gui.text_disabled.to_im_color();
        float x = box_pos.x + pad_x;
        const float y = row_top + ((box_end.y - row_top) - text_h) * 0.5f;
        for (size_t i = 0; i < segs.size(); ++i)
        {
            draw->AddText(ImVec2(x, y), segs[i].is_name ? name_col : col_text, segs[i].txt.c_str());
            x += CalcTextSize(segs[i].txt.c_str()).x;
            if (i + 1 < segs.size())
            {
                draw->AddText(ImVec2(x, y), col_sep, sep);
                x += sep_sz.x;
            }
        }
        return;
    }

    if (Vars::wmBackground)
        draw->AddRectFilled(box_pos, box_end, (ImU32)ImColor(0.f, 0.f, 0.f, 0.55f), rounding);

    if (style == 1)
    {
        if (Vars::wmGlow)
            for (int i = 4; i >= 1; --i)
                draw->AddRectFilled(
                    ImVec2(box_pos.x, box_pos.y - (float)i),
                    ImVec2(box_end.x, box_pos.y + 1.f + (float)i),
                    faded(0.05f), 0.f);
        draw_line(box_pos.x, box_end.x, box_pos.y, 1.5f);
    }
    else
    {
        if (Vars::wmGlow)
            for (int i = 5; i >= 1; --i)
            {
                const float e = (float)i * 2.f;
                draw->AddRect(
                    ImVec2(box_pos.x - e, box_pos.y - e),
                    ImVec2(box_end.x + e, box_end.y + e),
                    faded(0.045f / (float)i), rounding + e, 0, 2.f);
            }
        draw->AddRect(box_pos, box_end, accent_solid, rounding, 0, 1.5f);
    }

    const ImU32 col_text     = (ImU32)gui.text.to_im_color();
    const ImU32 col_disabled = (ImU32)gui.text_disabled.to_im_color();

    float x = box_pos.x + pad_x;
    const float y = box_pos.y + (box_sz.y - text_h) * 0.5f;
    for (size_t i = 0; i < segs.size(); ++i)
    {
        draw->AddText(ImVec2(x, y), segs[i].is_name ? name_col : col_text, segs[i].txt.c_str());
        x += CalcTextSize(segs[i].txt.c_str()).x;
        if (i + 1 < segs.size())
        {
            draw->AddText(ImVec2(x, y), col_disabled, sep);
            x += sep_sz.x;
        }
    }
}
