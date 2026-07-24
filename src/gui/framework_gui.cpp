
#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif

#include "framework_gui.h"
#include "framework_widgets.h"

#include "imgui.h"
#include "imgui_internal.h"

#include "hashes.hpp"   // ICON_FA_* macros (Font Awesome glyphs)
#include "bytes.hpp"    // museo500_binary / museo900_binary / font_awesome_binary / esliboganet

#include "vars.h"      // Vars::noclip / speedhack / fpsUnlock / etc.
#include "hooks.h"     // ApplyFPS()
#include "watermark.h" // DrawWatermark() / DrawWatermarkSettings()
#include "about.h"     // DrawAboutWindow() / g_aboutOpen
#include "binds.h"     // BindSystem / DrawBindPopup / DrawHotkeysList / DrawBindsOverlay
#include "../hooks/MacroEngine.hpp" // nh::MacroEngine

#include <string>
#include <cstring>
#include "../hooks/FrameAdvanceState.hpp"
#include <vector>

using namespace ImGui;

// Format an integer with comma thousands separators (e.g. 12684 -> "12,684").
static std::string nh_group_thousands(size_t n) {
    std::string s = std::to_string(n);
    int insert = static_cast<int>(s.size()) - 3;
    while (insert > 0) { s.insert(insert, ","); insert -= 3; }
    return s;
}

void FrameWorkInit()
{
    ImGuiIO& io = GetIO();

    ImGuiStyle& s = GetStyle();

    s.WindowRounding = 6.f;
    s.ChildRounding = 6.f;
    s.FrameRounding = 4.f;
    s.PopupRounding = 4.f;
    s.GrabRounding = 4.f;
    s.TabRounding = 4.f;
    s.ScrollbarRounding = 4.f;

    s.WindowBorderSize = 1.f;
    s.ChildBorderSize = 0.f;
    s.FrameBorderSize = 0.f;
    s.PopupBorderSize = 1.f;

    s.WindowPadding = ImVec2(8.f, 8.f);
    s.FramePadding = ImVec2(6.f, 4.f);
    s.ItemSpacing = ImVec2(8.f, 8.f);
    s.ItemInnerSpacing = ImVec2(6.f, 4.f);
    s.IndentSpacing = 18.f;
    s.ScrollbarSize = 8.f;
    s.GrabMinSize = 10.f;

    ImVec4* c = s.Colors;
    c[ImGuiCol_WindowBg] = ImVec4(0.012f, 0.020f, 0.045f, 1.f); // deepest
    c[ImGuiCol_ChildBg] = ImVec4(0.f, 0.f, 0.f, 0.f); // group_box paints its own
    c[ImGuiCol_PopupBg] = ImVec4(0.019f, 0.035f, 0.062f, 0.98f);

    c[ImGuiCol_Border] = ImVec4(1.f, 1.f, 1.f, 0.05f);
    c[ImGuiCol_BorderShadow] = ImVec4(0.f, 0.f, 0.f, 0.f);

    c[ImGuiCol_FrameBg] = gui.frame_inactive.to_vec4();
    c[ImGuiCol_FrameBgHovered] = gui.frame_active.to_vec4();
    c[ImGuiCol_FrameBgActive] = gui.frame_active.to_vec4();

    c[ImGuiCol_Text] = gui.text.to_vec4();
    c[ImGuiCol_TextDisabled] = gui.text_disabled.to_vec4();

    c[ImGuiCol_Button] = gui.button_bg.to_vec4();
    c[ImGuiCol_ButtonHovered] = gui.button_hovered.to_vec4();
    c[ImGuiCol_ButtonActive] = gui.button_active.to_vec4();

    c[ImGuiCol_Header] = gui.frame_active.to_vec4();
    c[ImGuiCol_HeaderHovered] = gui.frame_active.to_vec4();
    c[ImGuiCol_HeaderActive] = gui.frame_active.to_vec4();

    c[ImGuiCol_CheckMark] = gui.accent_color.to_vec4();
    c[ImGuiCol_SliderGrab] = gui.accent_color.to_vec4();
    c[ImGuiCol_SliderGrabActive] = gui.accent_color.to_vec4();

    c[ImGuiCol_ScrollbarBg] = ImVec4(0.f, 0.f, 0.f, 0.f);
    c[ImGuiCol_ScrollbarGrab] = ImVec4(1.f, 1.f, 1.f, 0.10f);
    c[ImGuiCol_ScrollbarGrabHovered] = ImVec4(1.f, 1.f, 1.f, 0.15f);
    c[ImGuiCol_ScrollbarGrabActive] = ImVec4(1.f, 1.f, 1.f, 0.20f);

    c[ImGuiCol_Separator] = ImVec4(1.f, 1.f, 1.f, 0.05f);
    c[ImGuiCol_SeparatorHovered] = ImVec4(1.f, 1.f, 1.f, 0.10f);
    c[ImGuiCol_SeparatorActive] = gui.accent_color.to_vec4();

    ImFontConfig cfg_regular;
    cfg_regular.FontDataOwnedByAtlas = false;
    io.Fonts->AddFontFromMemoryTTF(museo500_binary, (int)sizeof(museo500_binary), 14.f, &cfg_regular);

    static const ImWchar icon_ranges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
    ImFontConfig cfg_icons;
    cfg_icons.FontDataOwnedByAtlas = false;
    cfg_icons.MergeMode = true;
    cfg_icons.PixelSnapH = true;
    io.Fonts->AddFontFromMemoryTTF(font_awesome_binary, (int)sizeof(font_awesome_binary), 13.f, &cfg_icons, icon_ranges);

    // -- Fonts[1]: big header font (museo900 28px) used for "NEVERLOSE" -----
    ImFontConfig cfg_big;
    cfg_big.FontDataOwnedByAtlas = false;
    io.Fonts->AddFontFromMemoryTTF(museo900_binary, (int)sizeof(museo900_binary), 28.f, &cfg_big);
}

// ---------------------------------------------------------------------------
// FrameWorkShutdown -- phase-1 placeholder. Phase 2 will release the OpenGL
// FBO / textures / shader used by the blur subsystem here.
// ---------------------------------------------------------------------------
void FrameWorkShutdown()
{
    /* nothing to release in phase 1 */
}

// ---------------------------------------------------------------------------
// DrawFrameWorkGUI -- per-frame draw. Direct port of the in-loop code from
// neverlose-last/.../main.cpp lines ~125..290.
// ---------------------------------------------------------------------------
void DrawFrameWorkGUI()
{
    // Always-on overlay. It draws to the foreground draw list, so it must run
    // every frame regardless of the menu's open/closed/fade state. Keep it
    // ABOVE the fade gate below, otherwise it would vanish with the menu.
    // -- Menu Scale + Animation Speed (configured in the About window) --------
    {
        ImGuiIO& _io = GetIO();
        float uiScale;
        switch (Vars::menuScale) {
        case 1:  uiScale = 1.00f; break;
        case 2:  uiScale = 1.25f; break;
        case 3:  uiScale = 1.50f; break;
        case 4:  uiScale = 2.00f; break;
        default: uiScale = ImClamp(_io.DisplaySize.y / 1080.f, 0.85f, 1.75f); break; // Auto
        }
        _io.FontGlobalScale = uiScale;
    }
    g_menuAnimSpeed = Vars::menuAnimSpeed;

    BindSystem::get().process();
    DrawWatermark();
    BindSystem::get().drawBindsOverlay();
    BindSystem::get().drawBindPopup();
    BindSystem::get().drawHotkeysList();
    DrawAboutWindow();

    // Auto-Save: persist config the moment the menu is closed (if enabled).
    {
        static bool s_prevMenuOpen = false;
        if (s_prevMenuOpen && !Vars::menuOpen && Vars::autoSave)
            Config::get().save();
        s_prevMenuOpen = Vars::menuOpen;
    }

    // -- Fade in / out ---------------------------------------------------------
    // m_fade tracks 0..1 independently of menuOpen so we can still render
    // (and block input) during the closing animation.
    gui.m_fade = fi_lerp(gui.m_fade, Vars::menuOpen ? 1.f : 0.f, 0.40f);

    // Nothing to draw and no input to steal once fully faded out.
    if (gui.m_fade < 0.004f) {
        gui.m_anim = 0.f;   // reset per-tab anim so next open re-plays the slide-in
        return;
    }

    PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    // IMPORTANT: push Alpha BEFORE Begin() so the window border / bg / shadow
    // (drawn inside Begin() via GetColorU32(ImGuiCol_Border) which multiplies by
    // style.Alpha) also fades. Pushing it after Begin leaves a visible 1-px
    // outline ghost while the rest of the menu is fading out.
    PushStyleVar(ImGuiStyleVar_Alpha, gui.m_fade);
    SetNextWindowBgAlpha(gui.m_fade);  // backdrop alpha

    // First launch: place the menu in a sensible default position.
    // After first use, ImGui persists the user-dragged position via imgui.ini.
    SetNextWindowPos(ImVec2(100.f, 100.f), ImGuiCond_FirstUseEver);

    ImGui::Begin("##Neverhook", NULL, ImGuiWindowFlags_NoDecoration);
    {
        auto window = GetCurrentWindow();
        auto draw = window->DrawList;
        auto pos = window->Pos;
        auto size = window->Size;
        auto style = GetStyle(); (void)style;
        ImGuiIO& io = GetIO();

        gui.m_anim = fi_lerp(gui.m_anim, 1.f, 0.15f);

        SetWindowSize(ImVec2(690, 500));

        // Note: gui.m_fade is already applied via the outer PushStyleVar(Alpha)
        // before Begin(), so all to_im_color() calls in here automatically fade.
        // Raw ImColor() calls multiply by gui.m_fade explicitly below.

        // -- Left sidebar: solid fill, rounded to match WindowRounding so the
        //    rect doesn't poke out of the window's rounded corners.
        draw->AddRectFilled(
            pos, pos + ImVec2(170.f, size.y),
            ImColor(0.022f, 0.038f, 0.072f, gui.m_fade),
            6.f, ImDrawFlags_RoundCornersLeft
        );

        draw->AddLine(pos + ImVec2(170.f, 6.f), pos + ImVec2(170.f, size.y - 6.f),
            gui.border.to_im_color());

        draw->AddLine(pos + ImVec2(170.f, 60.f), pos + ImVec2(size.x - 6.f, 60.f),
            gui.border.to_im_color());


        if (io.Fonts->Fonts.Size > 1) {
            ImFont* big = io.Fonts->Fonts[1];
            const float  big_size = 28.f;
            const ImVec2 nl_size = big->CalcTextSizeA(big_size, FLT_MAX, 0, "NEVERHOOK");
            draw->AddText(big, big_size, pos + ImVec2(170 / 2 - nl_size.x / 2 + 1, 20), gui.accent_color.to_im_color(), "NEVERHOOK");
            draw->AddText(big, big_size, pos + ImVec2(170 / 2 - nl_size.x / 2, 20), GetColorU32(ImGuiCol_Text), "NEVERHOOK");
        }

        // -- Footer: separator + "Info" button (opens the About window)
        draw->AddLine(pos + ImVec2(8, size.y - 50), pos + ImVec2(162, size.y - 50),
            gui.border.to_im_color());

        SetCursorPos(ImVec2(10, size.y - 42));
        BeginChild("##infofoot", ImVec2(150, 28));
        if (gui.tab(ICON_FA_INFO_CIRCLE, "Info", g_aboutOpen))
            g_aboutOpen = !g_aboutOpen;
        EndChild();


        SetCursorPos(ImVec2(10, 70));
        BeginChild("##tabs", ImVec2(150, size.y - 120));

        gui.group_title("Aimbot");
        if (gui.tab(ICON_FA_CROSSHAIRS, "Player", gui.m_tab == 0) && gui.m_tab != 0)
            gui.m_tab = 0, gui.m_anim = 0.f;

        if (gui.tab(ICON_FA_GHOST, "Global", gui.m_tab == 1) && gui.m_tab != 1)
            gui.m_tab = 1, gui.m_anim = 0.f;

        if (gui.tab(ICON_FA_MOUSE, "Macros", gui.m_tab == 2) && gui.m_tab != 2)
            gui.m_tab = 2, gui.m_anim = 0.f;

        Spacing();

        gui.group_title("Visuals");
        if (gui.tab(ICON_FA_USER, "Cosmetic", gui.m_tab == 3) && gui.m_tab != 3)
            gui.m_tab = 3, gui.m_anim = 0.f;

        if (gui.tab(ICON_FA_PALLET, "Bypass", gui.m_tab == 4) && gui.m_tab != 4)
            gui.m_tab = 4, gui.m_anim = 0.f;

        Spacing();

        gui.group_title("Miscellaneous");
        if (gui.tab(ICON_FA_HAMMER, "Creator", gui.m_tab == 5) && gui.m_tab != 5)
            gui.m_tab = 5, gui.m_anim = 0.f;

        if (gui.tab(ICON_FA_CODE, "Scripts", gui.m_tab == 6) && gui.m_tab != 6)
            gui.m_tab = 6, gui.m_anim = 0.f;

        EndChild();

        SetCursorPos(ImVec2(190, 20));
        if (gui.button(ICON_FA_SAVE " Save", ImVec2(100, 25)))
            Config::get().save();

        PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

        SetCursorPos(ImVec2(300, 20));
        BeginChild("##subtabs", ImVec2(240, 25));

        GetWindowDrawList()->AddRectFilled(GetWindowPos(), GetWindowPos() + GetWindowSize(), gui.button_bg.to_im_color(), 4);
        GetWindowDrawList()->AddRect(GetWindowPos(), GetWindowPos() + GetWindowSize(), gui.border.to_im_color(), 4);

        for (int i = 0; i < (int)gui.rage_subtabs.size(); ++i) {

            ImDrawFlags flags = 0;
            if (i == 0)                                              flags = ImDrawFlags_RoundCornersLeft;
            else if (i == (int)gui.rage_subtabs.size() - 1)         flags = ImDrawFlags_RoundCornersRight;

            if (gui.subtab(gui.rage_subtabs.at(i), gui.m_rage_subtab == i, (int)gui.rage_subtabs.size(), flags) && gui.m_rage_subtab != i)
                gui.m_rage_subtab = i, gui.m_anim = 0.f;

            if (i != (int)gui.rage_subtabs.size() - 1)
                SameLine();
        }

        EndChild();

        PopStyleVar();

        // Multiply by m_fade so closing the menu also fades the content area
        // (ImGui Push replaces, it does NOT multiply, the previous alpha).
        PushStyleVar(ImGuiStyleVar_Alpha, gui.m_anim * gui.m_fade);
        PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 5));


        SetCursorPos(ImVec2(185, 81 - (5 * gui.m_anim)));
        BeginChild("##childs", ImVec2(size.x - 200, size.y - 96));

        switch (gui.m_tab) {

        case 0:

            gui.group_box(ICON_FA_USER " Player", ImVec2(GetWindowWidth(), GetWindowHeight())); {

                gui.toggle("Noclip", &Vars::noclip);
                gui.toggle("Noclip Hitsound", &Vars::noclipHitsound);
                if (Vars::noclipHitsound)
                    gui.slider_float("Hitsound Volume", &Vars::noclipHitsoundVolume, 0.f, 200.f, "%.0f%%");
                gui.toggle("Noclip Tint", &Vars::noclipTint);
                if (Vars::noclipTint) {
                    ImGui::ColorEdit4("Tint Color", Vars::noclipTintColor,
                        ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
                    gui.slider_float("Tint Opacity", &Vars::noclipTintOpacity, 0.f, 100.f, "%.0f%%");
                    gui.slider_float("Tint Fade", &Vars::noclipTintTime, 0.f, 3.f, "%.2fs");
                }
                gui.toggle("No Death Effect", &Vars::noDeathEffect);
                gui.toggle("No Respawn Flash", &Vars::noRespawnFlash);
                gui.toggle("No Pause Button", &Vars::noPauseButton);
                gui.toggle("Random Seed", &Vars::randomSeed);
                if (Vars::randomSeed)
                    gui.slider_int("Seed", &Vars::randomSeedValue, 0, 100000);

            } gui.end_group_box();

            break;

        case 1:

            gui.group_box(ICON_FA_RUNNING " Global", ImVec2(GetWindowWidth() / 2 - GetStyle().ItemSpacing.x / 2, GetWindowHeight())); {

                gui.toggle("Speedhack", &Vars::speedhack);
                if (Vars::speedhack) {
                    PushItemWidth(-1);
                    InputFloat("##speed", &Vars::speedhackValue, 0.f, 0.f, "%.2fx");
                    PopItemWidth();
                }
                gui.toggle("Speedhack Audio", &Vars::speedhackAudio);

            } gui.end_group_box();

            SameLine();

            gui.group_box(ICON_FA_TACHOMETER " Display", ImVec2(GetWindowWidth() / 2 - GetStyle().ItemSpacing.x / 2, GetWindowHeight())); {

                if (gui.toggle("FPS Bypass", &Vars::fpsUnlock))
                    ApplyFPS();

                if (Vars::fpsUnlock) {
                    PushItemWidth(-1);
                    if (InputFloat("##fps", &Vars::fpsValue, 0.f, 0.f, "%.0f FPS"))
                        ApplyFPS();
                    PopItemWidth();
                }

                // -- TPS Bypass (custom physics tick rate; user may type ANY value)
                Spacing();
                gui.toggle("TPS Bypass", &Vars::tpsBypass);
                if (Vars::tpsBypass) {
                    PushItemWidth(-1);
                    InputFloat("##tps", &Vars::tpsValue, 0.f, 0.f, "%.0f TPS");
                    PopItemWidth();
                }

                // -- Frame Extrapolation (placed below TPS Bypass)
                Spacing();
                gui.toggle("Frame Extrapolation", &Vars::frameExtrapolation);
                gui.toggle("Compact List", &Vars::compactList);
            } gui.end_group_box();

            break;

        case 2:  // Macros

            gui.group_box(ICON_FA_FILM " Macros", ImVec2(GetWindowWidth() / 2 - GetStyle().ItemSpacing.x / 2, GetWindowHeight())); {

                auto& eng = nh::MacroEngine::get();

                static char nameBuf[128] = "";
                static bool nameInit = false;
                if (!nameInit) {
                    std::strncpy(nameBuf, eng.currentName.c_str(), sizeof(nameBuf) - 1);
                    nameBuf[sizeof(nameBuf) - 1] = '\0';
                    nameInit = true;
                }

                // -- Macro name --
                TextDisabled("Name");
                PushItemWidth(-1);
                if (InputText("##macroName", nameBuf, sizeof(nameBuf)))
                    eng.currentName = nameBuf;
                PopItemWidth();

                // -- Saved macros dropdown --
                TextDisabled("Saved macros");
                PushItemWidth(-1);
                std::string preview = eng.currentName.empty() ? "(unnamed)" : eng.currentName;
                if (BeginCombo("##macroList", preview.c_str())) {
                    auto macros = eng.listMacros();
                    if (macros.empty())
                        TextDisabled("  no saved macros");
                    for (auto& n : macros) {
                        bool sel = (n == eng.currentName);
                        if (Selectable(n.c_str(), sel)) {
                            eng.load(n);
                            std::strncpy(nameBuf, n.c_str(), sizeof(nameBuf) - 1);
                            nameBuf[sizeof(nameBuf) - 1] = '\0';
                            eng.currentName = n;
                        }
                        if (sel) SetItemDefaultFocus();
                    }
                    EndCombo();
                }
                PopItemWidth();

                Spacing();
                Text("Actions: %s", nh_group_thousands(eng.actionCount()).c_str());
                Spacing();

                const float half = (GetContentRegionAvail().x - GetStyle().ItemSpacing.x) / 2.f;
                const bool rec  = eng.isRecording();
                const bool play = eng.isPlaying();

                // -- Record / Playback --
                if (gui.button(rec ? "Stop recording" : "Record", ImVec2(half, 0))) {
                    if (rec) {
                        eng.stop();
                        eng.save(eng.currentName);
                        if (Vars::macroAutoPlayback)
                            eng.startPlayback();
                    } else {
                        eng.startRecording();
                    }
                }
                SameLine();
                if (gui.button(play ? "Stop playback" : "Playback", ImVec2(half, 0))) {
                    if (play) eng.stop();
                    else      eng.startPlayback();
                }

                // -- Save / Load --
                if (gui.button("Save", ImVec2(half, 0)))
                    eng.save(eng.currentName);
                SameLine();
                if (gui.button("Load", ImVec2(half, 0)))
                    eng.load(eng.currentName);

                if (gui.button("Open Macros folder", ImVec2(-1, 0)))
                    eng.openFolder();

                Spacing();
                Separator();
                Spacing();

                // -- Options --
                TextDisabled("Playback on attempt (0 = instantly)");
                PushItemWidth(-1);
                if (InputInt("##macroAttempt", &Vars::macroPlaybackAttempt) &&
                    Vars::macroPlaybackAttempt < 0)
                    Vars::macroPlaybackAttempt = 0;
                PopItemWidth();

                gui.toggle("Ignore player inputs", &Vars::macroIgnoreInputs);
                gui.toggle("Auto playback after record", &Vars::macroAutoPlayback);

            } gui.end_group_box();

            SameLine();

            gui.group_box(ICON_FA_MOUSE " Autoclicker", ImVec2(GetWindowWidth() / 2 - GetStyle().ItemSpacing.x / 2, GetWindowHeight())); {

                gui.toggle("Player 1", &Vars::autoclicker);
                if (Vars::autoclicker)
                    gui.slider_float("P1 CPS", &Vars::autoclickerCps, 1.0f, 30.0f, "%.0f CPS");

                Spacing();
                gui.toggle("Player 2", &Vars::autoclickerP2);
                if (Vars::autoclickerP2)
                    gui.slider_float("P2 CPS", &Vars::autoclickerP2Cps, 1.0f, 30.0f, "%.0f CPS");

            } gui.end_group_box();

            break;

        case 3:  // Cosmetic

            gui.group_box(ICON_FA_USER " Visuals", ImVec2(GetWindowWidth() / 2 - GetStyle().ItemSpacing.x / 2, GetWindowHeight())); {

                gui.toggle("Hide Attempts", &Vars::hideAttempts);
                gui.toggle("No Glow", &Vars::noGlow);
                // gui.toggle("No Camera Shake", &Vars::noCameraShake);
                // gui.toggle("No End Shake", &Vars::noEndShake);
                gui.toggle("No Dash Fire", &Vars::noDashFire);
                gui.toggle("No Spider Dash", &Vars::noSpiderDash);
                gui.toggle("No Particles", &Vars::noParticles);
                gui.toggle("No Trail", &Vars::noTrail);
                gui.toggle("Hide Player", &Vars::hidePlayer);
                gui.toggle("Player On Top", &Vars::playerOnTop);
                gui.toggle("No Robot Fire", &Vars::noRobotFire);
                gui.toggle("No Swing Fire", &Vars::noSwingFire);
                gui.toggle("No Ghost Trail", &Vars::noGhostTrail);
                gui.toggle("No Trail Behind Wave", &Vars::noTrailBehindWave);
                gui.toggle("No Circle Wave", &Vars::noCircleWave);

                Spacing();
                gui.toggle("No Wave Pulse", &Vars::noWavePulse);
                gui.toggle("No Wave Trail", &Vars::noWaveTrail);
                gui.toggle("Solid Wave Trail", &Vars::solidWaveTrail);
                gui.toggle("Wave Trail Size", &Vars::waveTrailSize);
                gui.toggle("No Shader", &Vars::noShader);
                // gui.toggle("No Portal Lightning", &Vars::noPortalLightning);
                gui.toggle("Hide Complete VFX", &Vars::hideLevelCompleteVfx);
                gui.toggle("No Music Fade Out", &Vars::noMusicFadeOut);
                if (Vars::waveTrailSize)
                    gui.slider_float("Size", &Vars::waveTrailSizeValue, 0.1f, 5.0f, "%.1fx");

                gui.toggle("Accurate Percentage", &Vars::accuratePercent);
                if (Vars::accuratePercent)
                    gui.slider_int("Decimals", &Vars::accuratePercentDigits, 2, 4);

                Spacing();
                DrawWatermarkSettings();

                Spacing();
                gui.toggle("Keybinds", &Vars::keybindsList);
                {
                    static float kbExpand = 0.f;
                    const float kbDt = GetIO().DeltaTime > 0.f ? GetIO().DeltaTime : 1.f / 60.f;
                    kbExpand += ((Vars::keybindsList ? 1.f : 0.f) - kbExpand) * ImMin(1.f, kbDt * 12.f);
                    if (kbExpand > 0.004f) {
                        static float kbFullH = 52.f;
                        BeginChild("##kbStyleGroup", ImVec2(GetContentRegionAvail().x, kbFullH * kbExpand),
                                   false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
                        PushStyleVar(ImGuiStyleVar_Alpha, GetStyle().Alpha * kbExpand);
                        PushID("keybindsStyle");
                        static const char* kb_styles[] = { "Version 1.5", "Onetap v3", "Skeet" };
                        wm_combo("Style", &Vars::keybindsStyle, kb_styles, IM_ARRAYSIZE(kb_styles));
                        static const char* kb_lines[] = { "Static", "Gradient" };
                        wm_combo("Line", &Vars::keybindsLine, kb_lines, IM_ARRAYSIZE(kb_lines));
                        const float kbY1 = GetCursorPosY();
                        PopID();
                        PopStyleVar();
                        EndChild();
                        if (Vars::keybindsList) kbFullH = ImMax(28.f, kbY1 + 4.f);
                    }
                }

            } gui.end_group_box();

            SameLine();

            gui.group_box(ICON_FA_PALLET " Hitboxes", ImVec2(GetWindowWidth() / 2 - GetStyle().ItemSpacing.x / 2, GetWindowHeight())); {

                gui.toggle("Show Hitboxes", &Vars::showHitboxes);
                gui.toggle("Show On Death", &Vars::showHitboxesOnDeath);
                gui.toggle("Trajectory Prediction", &Vars::showTrajectory);

                Spacing();
                gui.toggle("Hitbox Multiplier", &Vars::hitboxMultiplier);
                if (Vars::hitboxMultiplier) {
                    gui.slider_float("Player",  &Vars::hitboxMultPlayer, 0.1f, 3.0f, "%.1fx");
                    gui.slider_float("Solids",  &Vars::hitboxMultSolid,  0.1f, 3.0f, "%.1fx");
                    gui.slider_float("Hazards", &Vars::hitboxMultHazard, 0.1f, 3.0f, "%.1fx");
                }

            } gui.end_group_box();

            break;

        case 4:  // Bypass

            gui.group_box(ICON_FA_KEY " Bypass", ImVec2(GetWindowWidth() / 2 - GetStyle().ItemSpacing.x / 2, GetWindowHeight())); {

                gui.toggle("Practice Music", &Vars::practiceMusic);
                gui.toggle("Practice Fix", &Vars::practiceFix);
                gui.toggle("Icon Bypass", &Vars::iconBypass);
                gui.toggle("Unlock Main Levels", &Vars::unlockMainLevels);
                gui.toggle("Unlock Shops", &Vars::unlockShops);
                gui.toggle("Unlock Vaults", &Vars::unlockVaults);
                gui.toggle("No Transition", &Vars::noTransition);

                Spacing();
                gui.toggle("Safe Mode", &Vars::safeMode);
                if (Vars::safeMode) {
                    gui.toggle("Freeze Attempts", &Vars::safeFreezeAttempts);
                    gui.toggle("Freeze Jumps", &Vars::safeFreezeJumps);
                }

            } gui.end_group_box();

            SameLine();

            gui.group_box(ICON_FA_FLAG_CHECKERED " Level", ImVec2(GetWindowWidth() / 2 - GetStyle().ItemSpacing.x / 2, GetWindowHeight())); {

                gui.toggle("Instant Complete", &Vars::instantComplete);
                gui.toggle("No Mirror Portal", &Vars::noMirrorPortal);
                gui.toggle("Smart StartPos", &Vars::smartStartpos);
                gui.toggle("StartPos Switcher", &Vars::startposSwitcher);
                gui.toggle("Frame Advance", &Vars::frameAdvance);
                if (Vars::frameAdvance) {
                    char btnLabel[48];
                    int k = Vars::faStepKey;
                    if (nh::faKeyWaiting)
                        snprintf(btnLabel, sizeof(btnLabel), "Step Key: [press key...]");
                    else if (k >= 65 && k <= 90)
                        snprintf(btnLabel, sizeof(btnLabel), "Step Key: [%c]", (char)k);
                    else if (k >= 48 && k <= 57)
                        snprintf(btnLabel, sizeof(btnLabel), "Step Key: [%c]", (char)k);
                    else
                        snprintf(btnLabel, sizeof(btnLabel), "Step Key: [#%d]", k);
                    PushItemWidth(-1);
                    if (Button(btnLabel, ImVec2(-1, 0)))
                        nh::faKeyWaiting = true;
                    PopItemWidth();

                    gui.toggle("Hold to Step", &Vars::faHold);
                    if (Vars::faHold) {
                        PushItemWidth(-1);
                        InputFloat("##faHoldDelay", &Vars::faHoldDelayCfg, 0.f, 0.f, "%.2f s delay");
                        InputInt("##faHoldSpeed", &Vars::faHoldSpeedCfg);
                        PopItemWidth();
                    }
                }

                Spacing();
                gui.toggle("Instant Restart", &Vars::instantRestart);
                gui.toggle("Custom Respawn", &Vars::customRespawn);
                if (Vars::customRespawn && !Vars::instantRestart) {
                    PushItemWidth(-1);
                    InputFloat("##respawn", &Vars::respawnTime, 0.f, 0.f, "%.2f s");
                    PopItemWidth();
                }

                Spacing();
                gui.toggle("Jump Hack", &Vars::jumpHack);
                gui.toggle("All Modes Platformer", &Vars::allModesPlatformer);
                gui.toggle("Auto Practice Mode", &Vars::autoPracticeMode);
                gui.toggle("Auto Pickup Coins", &Vars::autoPickupCoins);
                gui.toggle("Pause On Complete", &Vars::pauseDuringComplete);
                gui.toggle("Auto Song Download", &Vars::autoSongDownload);
                gui.toggle("Layout Mode", &Vars::layoutMode);

            } gui.end_group_box();

            break;

        case 5:  // Creator

            gui.group_box(ICON_FA_HAMMER " Creator", ImVec2(GetWindowWidth(), GetWindowHeight())); {

                gui.toggle("Verify Hack", &Vars::verifyHack);
                gui.toggle("Copy Hack", &Vars::copyHack);

                Spacing();
                gui.toggle("Hide Editor UI", &Vars::hideEditorUI);
                gui.toggle("Level Edit", &Vars::levelEdit);
                gui.toggle("No Custom Obj Limit", &Vars::noCustomObjLimit);
                gui.toggle("No Zoom Limit", &Vars::noZoomLimit);
                gui.toggle("Toolbox Button Bypass", &Vars::toolboxButtonBypass);
                gui.toggle("Slider Limit Bypass", &Vars::sliderLimitBypass);

            } gui.end_group_box();

            break;

        case 6:  // Scripts  -- placeholder, no widgets yet
        default:
            break;
        }

        EndChild();

        PopStyleVar(2);
    }
    ImGui::End();

    PopStyleVar();  // m_fade alpha (matches Push before Begin)
    PopStyleVar();  // WindowPadding
}