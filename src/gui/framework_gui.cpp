
#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif

#include "framework_gui.h"
#include "framework_widgets.h"

#include "imgui.h"
#include "imgui_internal.h"
#include <imgui-cocos.hpp>

#include "hashes.hpp"
#include "bytes.hpp"

#include "vars.h"
#include "hooks.h"
#include "watermark.h"
#include "StatusOverlay.h"
#include "about.h"
#include "binds.h"
#include "scripts_tab.h"
#include "blur.hpp"
#include "../hooks/MacroEngine.hpp"

#include <string>
#include <cstring>
#include "../hooks/FrameAdvanceState.hpp"
#include <vector>

using namespace ImGui;

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
    c[ImGuiCol_WindowBg] = ImVec4(0.012f, 0.020f, 0.045f, 1.f);
    c[ImGuiCol_ChildBg] = ImVec4(0.f, 0.f, 0.f, 0.f);
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

    ImFontConfig cfg_big;
    cfg_big.FontDataOwnedByAtlas = false;
    io.Fonts->AddFontFromMemoryTTF(museo900_binary, (int)sizeof(museo900_binary), 28.f, &cfg_big);
}

void FrameWorkShutdown()
{

}

inline char g_featureSearch[64] = "";

inline bool MatchesFeatureQuery(const char* label, const char* query) {
    if (!query || query[0] == '\0') return true;
    if (!label) return false;
    std::string l = label;
    std::string q = query;
    for (auto& c : l) c = (char)::tolower((unsigned char)c);
    for (auto& c : q) c = (char)::tolower((unsigned char)c);
    return l.find(q) != std::string::npos;
}

static void DrawFeatureSearchResults(const char* q) {
    int totalMatches = 0;

    {
        int pMatches = 0;
        if (MatchesFeatureQuery("Noclip", q)) pMatches++;
        if (MatchesFeatureQuery("Noclip Tint", q)) pMatches++;
        if (MatchesFeatureQuery("No Death Effect", q)) pMatches++;
        if (MatchesFeatureQuery("No Respawn Flash", q)) pMatches++;
        if (MatchesFeatureQuery("No Pause Button", q)) pMatches++;
        if (MatchesFeatureQuery("Random Seed", q)) pMatches++;
        if (MatchesFeatureQuery("Show Hitboxes", q)) pMatches++;
        if (MatchesFeatureQuery("Show On Death", q)) pMatches++;
        if (MatchesFeatureQuery("Show Trajectory (WIP)", q)) pMatches++;
        if (MatchesFeatureQuery("Click Between Frames", q)) pMatches++;
        if (MatchesFeatureQuery("Hitbox Multiplier", q)) pMatches++;
        if (MatchesFeatureQuery("Instant Complete", q)) pMatches++;
        if (MatchesFeatureQuery("Smart StartPos", q)) pMatches++;
        if (MatchesFeatureQuery("StartPos Switcher", q)) pMatches++;
        if (MatchesFeatureQuery("Frame Advance", q)) pMatches++;
        if (MatchesFeatureQuery("Jump Hack", q)) pMatches++;
        if (MatchesFeatureQuery("All Modes Platformer", q)) pMatches++;
        if (MatchesFeatureQuery("Auto Practice Mode", q)) pMatches++;
        if (MatchesFeatureQuery("Auto Pickup Coins", q)) pMatches++;
        if (MatchesFeatureQuery("Auto Song Download", q)) pMatches++;
        if (MatchesFeatureQuery("Layout Mode", q)) pMatches++;

        if (pMatches > 0) {
            totalMatches += pMatches;
            const float boxH = pMatches * 26.f + 38.f;
            gui.group_box(ICON_FA_USER " Player", ImVec2(GetWindowWidth() - 14.f, boxH)); {
                if (MatchesFeatureQuery("Noclip", q)) gui.toggle("Noclip", &Vars::noclip);
                if (MatchesFeatureQuery("Noclip Tint", q)) {
                    gui.toggle("Noclip Tint", &Vars::noclipTint);
                    if (Vars::noclipTint) {
                        ImGui::ColorEdit4("Tint Color", Vars::noclipTintColor,
                            ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
                        gui.slider_float("Tint Opacity", &Vars::noclipTintOpacity, 0.f, 100.f, "%.0f%%");
                        gui.slider_float("Tint Fade", &Vars::noclipTintTime, 0.f, 3.f, "%.2fs");
                    }
                }
                if (MatchesFeatureQuery("No Death Effect", q)) gui.toggle("No Death Effect", &Vars::noDeathEffect);
                if (MatchesFeatureQuery("No Respawn Flash", q)) gui.toggle("No Respawn Flash", &Vars::noRespawnFlash);
                if (MatchesFeatureQuery("No Pause Button", q)) gui.toggle("No Pause Button", &Vars::noPauseButton);
                if (MatchesFeatureQuery("Random Seed", q)) {
                    gui.toggle("Random Seed", &Vars::randomSeed);
                    if (Vars::randomSeed) gui.slider_int("Seed", &Vars::randomSeedValue, 0, 100000);
                }
                if (MatchesFeatureQuery("Show Hitboxes", q)) gui.toggle("Show Hitboxes", &Vars::showHitboxes);
                if (MatchesFeatureQuery("Show On Death", q)) gui.toggle("Show On Death", &Vars::showHitboxesOnDeath);
                if (MatchesFeatureQuery("Show Trajectory (WIP)", q)) gui.toggle("Show Trajectory (WIP)", &Vars::showTrajectory);
                if (MatchesFeatureQuery("Click Between Frames", q)) gui.toggle("Click Between Frames", &Vars::clickBetweenFrames);
                if (MatchesFeatureQuery("Hitbox Multiplier", q)) {
                    gui.toggle("Hitbox Multiplier", &Vars::hitboxMultiplier);
                    if (Vars::hitboxMultiplier) {
                        gui.slider_float("Player",  &Vars::hitboxMultPlayer, 0.1f, 3.0f, "%.1fx");
                        gui.slider_float("Solids",  &Vars::hitboxMultSolid,  0.1f, 3.0f, "%.1fx");
                        gui.slider_float("Hazards", &Vars::hitboxMultHazard, 0.1f, 3.0f, "%.1fx");
                    }
                }
                if (MatchesFeatureQuery("Instant Complete", q)) gui.toggle("Instant Complete", &Vars::instantComplete);
                if (MatchesFeatureQuery("Smart StartPos", q)) gui.toggle("Smart StartPos", &Vars::smartStartpos);
                if (MatchesFeatureQuery("StartPos Switcher", q)) gui.toggle("StartPos Switcher", &Vars::startposSwitcher);
                if (MatchesFeatureQuery("Frame Advance", q)) gui.toggle("Frame Advance", &Vars::frameAdvance);
                if (MatchesFeatureQuery("Jump Hack", q)) gui.toggle("Jump Hack", &Vars::jumpHack);
                if (MatchesFeatureQuery("All Modes Platformer", q)) gui.toggle("All Modes Platformer", &Vars::allModesPlatformer);
                if (MatchesFeatureQuery("Auto Practice Mode", q)) gui.toggle("Auto Practice Mode", &Vars::autoPracticeMode);
                if (MatchesFeatureQuery("Auto Pickup Coins", q)) gui.toggle("Auto Pickup Coins", &Vars::autoPickupCoins);
                if (MatchesFeatureQuery("Auto Song Download", q)) gui.toggle("Auto Song Download", &Vars::autoSongDownload);
                if (MatchesFeatureQuery("Layout Mode", q)) gui.toggle("Layout Mode", &Vars::layoutMode);
            } gui.end_group_box();
            Dummy(ImVec2(0.f, 8.f));
        }
    }

    {
        int gMatches = 0;
        if (MatchesFeatureQuery("Speedhack", q)) gMatches++;
        if (MatchesFeatureQuery("Speedhack Audio", q)) gMatches++;
        if (MatchesFeatureQuery("FPS Bypass", q)) gMatches++;
        if (MatchesFeatureQuery("TPS Bypass", q)) gMatches++;
        if (MatchesFeatureQuery("Frame Extrapolation", q)) gMatches++;
        if (MatchesFeatureQuery("Compact List", q)) gMatches++;
        if (MatchesFeatureQuery("Endscreen Stats", q)) gMatches++;
        if (MatchesFeatureQuery("Endscreen Phrases", q)) gMatches++;

        if (gMatches > 0) {
            totalMatches += gMatches;
            const float boxH = gMatches * 26.f + 38.f;
            gui.group_box(ICON_FA_RUNNING " Global", ImVec2(GetWindowWidth() - 14.f, boxH)); {
                if (MatchesFeatureQuery("Speedhack", q)) {
                    gui.toggle("Speedhack", &Vars::speedhack);
                    if (Vars::speedhack) {
                        PushItemWidth(-1);
                        InputFloat("##speed", &Vars::speedhackValue, 0.f, 0.f, "%.2fx");
                        PopItemWidth();
                    }
                }
                if (MatchesFeatureQuery("Speedhack Audio", q)) gui.toggle("Speedhack Audio", &Vars::speedhackAudio);
                if (MatchesFeatureQuery("FPS Bypass", q)) {
                    if (gui.toggle("FPS Bypass", &Vars::fpsUnlock)) ApplyFPS();
                    if (Vars::fpsUnlock) {
                        PushItemWidth(-1);
                        if (InputFloat("##fps", &Vars::fpsValue, 0.f, 0.f, "%.0f FPS")) ApplyFPS();
                        PopItemWidth();
                    }
                }
                if (MatchesFeatureQuery("TPS Bypass", q)) {
                    gui.toggle("TPS Bypass", &Vars::tpsBypass);
                    if (Vars::tpsBypass) {
                        PushItemWidth(-1);
                        InputFloat("##tps", &Vars::tpsValue, 0.f, 0.f, "%.0f TPS");
                        PopItemWidth();
                    }
                }
                if (MatchesFeatureQuery("Frame Extrapolation", q)) gui.toggle("Frame Extrapolation", &Vars::frameExtrapolation);
                if (MatchesFeatureQuery("Compact List", q)) gui.toggle("Compact List", &Vars::compactList);
                if (MatchesFeatureQuery("Endscreen Stats", q)) gui.toggle("Endscreen Stats", &Vars::endscreenStats);
                if (MatchesFeatureQuery("Endscreen Phrases", q)) gui.toggle("Endscreen Phrases", &Vars::endscreenPhrases);
            } gui.end_group_box();
            Dummy(ImVec2(0.f, 8.f));
        }
    }

    {
        int vMatches = 0;
        if (MatchesFeatureQuery("Hide Attempts", q)) vMatches++;
        if (MatchesFeatureQuery("No Glow", q)) vMatches++;
        if (MatchesFeatureQuery("No Dash Fire", q)) vMatches++;
        if (MatchesFeatureQuery("No Spider Dash", q)) vMatches++;
        if (MatchesFeatureQuery("No Particles", q)) vMatches++;
        if (MatchesFeatureQuery("No Trail", q)) vMatches++;
        if (MatchesFeatureQuery("Hide Player", q)) vMatches++;
        if (MatchesFeatureQuery("Player On Top", q)) vMatches++;
        if (MatchesFeatureQuery("No Robot Fire", q)) vMatches++;
        if (MatchesFeatureQuery("No Swing Fire", q)) vMatches++;
        if (MatchesFeatureQuery("No Ghost Trail", q)) vMatches++;
        if (MatchesFeatureQuery("No Trail Behind Wave", q)) vMatches++;
        if (MatchesFeatureQuery("No Circle Wave", q)) vMatches++;
        if (MatchesFeatureQuery("No Wave Pulse", q)) vMatches++;
        if (MatchesFeatureQuery("No Wave Trail", q)) vMatches++;
        if (MatchesFeatureQuery("Solid Wave Trail", q)) vMatches++;
        if (MatchesFeatureQuery("Wave Trail Size", q)) vMatches++;
        if (MatchesFeatureQuery("No Shader", q)) vMatches++;
        if (MatchesFeatureQuery("No Camera Shake", q)) vMatches++;
        if (MatchesFeatureQuery("No End Shake", q)) vMatches++;
        if (MatchesFeatureQuery("No Death Shake", q)) vMatches++;
        if (MatchesFeatureQuery("Hide Complete VFX", q)) vMatches++;
        if (MatchesFeatureQuery("No Music Fade Out", q)) vMatches++;
        if (MatchesFeatureQuery("Accurate Percentage", q)) vMatches++;
        if (MatchesFeatureQuery("Keybinds", q)) vMatches++;

        if (vMatches > 0) {
            totalMatches += vMatches;
            const float boxH = vMatches * 26.f + 38.f;
            gui.group_box(ICON_FA_USER " Visuals", ImVec2(GetWindowWidth() - 14.f, boxH)); {
                if (MatchesFeatureQuery("Hide Attempts", q)) gui.toggle("Hide Attempts", &Vars::hideAttempts);
                if (MatchesFeatureQuery("No Glow", q)) gui.toggle("No Glow", &Vars::noGlow);
                if (MatchesFeatureQuery("No Dash Fire", q)) gui.toggle("No Dash Fire", &Vars::noDashFire);
                if (MatchesFeatureQuery("No Spider Dash", q)) gui.toggle("No Spider Dash", &Vars::noSpiderDash);
                if (MatchesFeatureQuery("No Particles", q)) gui.toggle("No Particles", &Vars::noParticles);
                if (MatchesFeatureQuery("No Trail", q)) gui.toggle("No Trail", &Vars::noTrail);
                if (MatchesFeatureQuery("Hide Player", q)) gui.toggle("Hide Player", &Vars::hidePlayer);
                if (MatchesFeatureQuery("Player On Top", q)) gui.toggle("Player On Top", &Vars::playerOnTop);
                if (MatchesFeatureQuery("No Robot Fire", q)) gui.toggle("No Robot Fire", &Vars::noRobotFire);
                if (MatchesFeatureQuery("No Swing Fire", q)) gui.toggle("No Swing Fire", &Vars::noSwingFire);
                if (MatchesFeatureQuery("No Ghost Trail", q)) gui.toggle("No Ghost Trail", &Vars::noGhostTrail);
                if (MatchesFeatureQuery("No Trail Behind Wave", q)) gui.toggle("No Trail Behind Wave", &Vars::noTrailBehindWave);
                if (MatchesFeatureQuery("No Circle Wave", q)) gui.toggle("No Circle Wave", &Vars::noCircleWave);
                if (MatchesFeatureQuery("No Wave Pulse", q)) gui.toggle("No Wave Pulse", &Vars::noWavePulse);
                if (MatchesFeatureQuery("No Wave Trail", q)) gui.toggle("No Wave Trail", &Vars::noWaveTrail);
                if (MatchesFeatureQuery("Solid Wave Trail", q)) gui.toggle("Solid Wave Trail", &Vars::solidWaveTrail);
                if (MatchesFeatureQuery("Wave Trail Size", q)) {
                    gui.toggle("Wave Trail Size", &Vars::waveTrailSize);
                    if (Vars::waveTrailSize)
                        gui.slider_float("Size", &Vars::waveTrailSizeValue, 0.1f, 5.0f, "%.1fx");
                }
                if (MatchesFeatureQuery("No Shader", q)) gui.toggle("No Shader", &Vars::noShader);
                if (MatchesFeatureQuery("No Camera Shake", q)) gui.toggle("No Camera Shake", &Vars::noCameraShake);
                if (MatchesFeatureQuery("No End Shake", q)) gui.toggle("No End Shake", &Vars::noEndShake);
                if (MatchesFeatureQuery("No Death Shake", q)) gui.toggle("No Death Shake", &Vars::noDeathShake);
                if (MatchesFeatureQuery("Hide Complete VFX", q)) gui.toggle("Hide Complete VFX", &Vars::hideLevelCompleteVfx);
                if (MatchesFeatureQuery("No Music Fade Out", q)) gui.toggle("No Music Fade Out", &Vars::noMusicFadeOut);
                if (MatchesFeatureQuery("Accurate Percentage", q)) {
                    gui.toggle("Accurate Percentage", &Vars::accuratePercent);
                    if (Vars::accuratePercent)
                        gui.slider_int("Decimals", &Vars::accuratePercentDigits, 2, 4);
                }
                if (MatchesFeatureQuery("Keybinds", q)) gui.toggle("Keybinds", &Vars::keybindsList);
            } gui.end_group_box();
            Dummy(ImVec2(0.f, 8.f));
        }
    }

    {
        int bMatches = 0;
        if (MatchesFeatureQuery("Practice Music", q)) bMatches++;
        if (MatchesFeatureQuery("Practice Fix", q)) bMatches++;
        if (MatchesFeatureQuery("Icon Bypass", q)) bMatches++;
        if (MatchesFeatureQuery("Unlock Main Levels", q)) bMatches++;
        if (MatchesFeatureQuery("Unlock Shops", q)) bMatches++;
        if (MatchesFeatureQuery("Unlock Vaults", q)) bMatches++;
        if (MatchesFeatureQuery("No Transition", q)) bMatches++;
        if (MatchesFeatureQuery("Safe Mode", q)) bMatches++;
        if (MatchesFeatureQuery("No Mirror Portal", q)) bMatches++;
        if (MatchesFeatureQuery("Instant Restart", q)) bMatches++;
        if (MatchesFeatureQuery("Custom Respawn", q)) bMatches++;
        if (MatchesFeatureQuery("Pause On Complete", q)) bMatches++;
        if (MatchesFeatureQuery("Hide Pause Menu", q)) bMatches++;
        if (MatchesFeatureQuery("Mouse Zoom on Pause", q)) bMatches++;

        if (bMatches > 0) {
            totalMatches += bMatches;
            const float boxH = bMatches * 26.f + 38.f;
            gui.group_box(ICON_FA_KEY " Bypass", ImVec2(GetWindowWidth() - 14.f, boxH)); {
                if (MatchesFeatureQuery("Practice Music", q)) gui.toggle("Practice Music", &Vars::practiceMusic);
                if (MatchesFeatureQuery("Practice Fix", q)) gui.toggle("Practice Fix", &Vars::practiceFix);
                if (MatchesFeatureQuery("Icon Bypass", q)) gui.toggle("Icon Bypass", &Vars::iconBypass);
                if (MatchesFeatureQuery("Unlock Main Levels", q)) gui.toggle("Unlock Main Levels", &Vars::unlockMainLevels);
                if (MatchesFeatureQuery("Unlock Shops", q)) gui.toggle("Unlock Shops", &Vars::unlockShops);
                if (MatchesFeatureQuery("Unlock Vaults", q)) gui.toggle("Unlock Vaults", &Vars::unlockVaults);
                if (MatchesFeatureQuery("No Transition", q)) gui.toggle("No Transition", &Vars::noTransition);
                if (MatchesFeatureQuery("Safe Mode", q)) gui.toggle("Safe Mode", &Vars::safeMode);
                if (MatchesFeatureQuery("No Mirror Portal", q)) gui.toggle("No Mirror Portal", &Vars::noMirrorPortal);
                if (MatchesFeatureQuery("Instant Restart", q)) gui.toggle("Instant Restart", &Vars::instantRestart);
                if (MatchesFeatureQuery("Custom Respawn", q)) {
                    gui.toggle("Custom Respawn", &Vars::customRespawn);
                    if (Vars::customRespawn && !Vars::instantRestart)
                        gui.slider_float("Respawn Time", &Vars::respawnTime, 0.05f, 5.0f, "%.2fs");
                }
                if (MatchesFeatureQuery("Pause On Complete", q)) gui.toggle("Pause On Complete", &Vars::pauseDuringComplete);
                if (MatchesFeatureQuery("Hide Pause Menu", q)) gui.toggle("Hide Pause Menu", &Vars::hidePauseMenu);
                if (MatchesFeatureQuery("Mouse Zoom on Pause", q)) {
                    const bool zoomModLoaded = geode::Loader::get()->isModLoaded("bobby_shmurner.zoom");
                    gui.toggle("Mouse Zoom on Pause", &Vars::mouseZoomOnPause);
                    if (zoomModLoaded) TextDisabled("Zoooom! mod is already active");
                }
            } gui.end_group_box();
            Dummy(ImVec2(0.f, 8.f));
        }
    }

    {
        int cMatches = 0;
        if (MatchesFeatureQuery("Verify Hack", q)) cMatches++;
        if (MatchesFeatureQuery("Copy Hack", q)) cMatches++;
        if (MatchesFeatureQuery("Hide Editor UI", q)) cMatches++;
        if (MatchesFeatureQuery("Level Edit", q)) cMatches++;
        if (MatchesFeatureQuery("No Custom Obj Limit", q)) cMatches++;
        if (MatchesFeatureQuery("No Zoom Limit", q)) cMatches++;
        if (MatchesFeatureQuery("Toolbox Button Bypass", q)) cMatches++;
        if (MatchesFeatureQuery("Slider Limit Bypass", q)) cMatches++;

        if (cMatches > 0) {
            totalMatches += cMatches;
            const float boxH = cMatches * 26.f + 38.f;
            gui.group_box(ICON_FA_HAMMER " Creator", ImVec2(GetWindowWidth() - 14.f, boxH)); {
                if (MatchesFeatureQuery("Verify Hack", q)) gui.toggle("Verify Hack", &Vars::verifyHack);
                if (MatchesFeatureQuery("Copy Hack", q)) gui.toggle("Copy Hack", &Vars::copyHack);
                if (MatchesFeatureQuery("Hide Editor UI", q)) gui.toggle("Hide Editor UI", &Vars::hideEditorUI);
                if (MatchesFeatureQuery("Level Edit", q)) gui.toggle("Level Edit", &Vars::levelEdit);
                if (MatchesFeatureQuery("No Custom Obj Limit", q)) gui.toggle("No Custom Obj Limit", &Vars::noCustomObjLimit);
                if (MatchesFeatureQuery("No Zoom Limit", q)) gui.toggle("No Zoom Limit", &Vars::noZoomLimit);
                if (MatchesFeatureQuery("Toolbox Button Bypass", q)) gui.toggle("Toolbox Button Bypass", &Vars::toolboxButtonBypass);
                if (MatchesFeatureQuery("Slider Limit Bypass", q)) gui.toggle("Slider Limit Bypass", &Vars::sliderLimitBypass);
            } gui.end_group_box();
            Dummy(ImVec2(0.f, 8.f));
        }
    }

    {
        int mMatches = 0;
        if (MatchesFeatureQuery("Autoclicker Player 1", q) || MatchesFeatureQuery("Autoclicker", q)) mMatches++;
        if (MatchesFeatureQuery("Autoclicker Player 2", q)) mMatches++;
        if (MatchesFeatureQuery("Ignore player inputs", q)) mMatches++;
        if (MatchesFeatureQuery("Auto playback after record", q)) mMatches++;

        if (mMatches > 0) {
            totalMatches += mMatches;
            const float boxH = mMatches * 26.f + 38.f;
            gui.group_box(ICON_FA_MOUSE " Macros / Autoclicker", ImVec2(GetWindowWidth() - 14.f, boxH)); {
                if (MatchesFeatureQuery("Autoclicker Player 1", q) || MatchesFeatureQuery("Autoclicker", q)) {
                    gui.toggle("Player 1", &Vars::autoclicker);
                    if (Vars::autoclicker)
                        gui.slider_float("P1 CPS", &Vars::autoclickerCps, 1.0f, 30.0f, "%.0f CPS");
                }
                if (MatchesFeatureQuery("Autoclicker Player 2", q)) {
                    gui.toggle("Player 2", &Vars::autoclickerP2);
                    if (Vars::autoclickerP2)
                        gui.slider_float("P2 CPS", &Vars::autoclickerP2Cps, 1.0f, 30.0f, "%.0f CPS");
                }
                if (MatchesFeatureQuery("Ignore player inputs", q)) gui.toggle("Ignore player inputs", &Vars::macroIgnoreInputs);
                if (MatchesFeatureQuery("Auto playback after record", q)) gui.toggle("Auto playback after record", &Vars::macroAutoPlayback);
            } gui.end_group_box();
            Dummy(ImVec2(0.f, 8.f));
        }
    }

    if (totalMatches == 0) {
        Dummy(ImVec2(0.f, 20.f));
        TextDisabled("  No features found matching '%s'.", q);
    }
}

void DrawFrameWorkGUI()
{

    {
        ImGuiIO& _io = GetIO();
        float uiScale;
        switch (Vars::menuScale) {
        case 1:  uiScale = 1.00f; break;
        case 2:  uiScale = 1.25f; break;
        case 3:  uiScale = 1.50f; break;
        case 4:  uiScale = 2.00f; break;
        default: uiScale = ImClamp(_io.DisplaySize.y / 1080.f, 0.85f, 1.75f); break;
        }
        _io.FontGlobalScale = uiScale;
    }
    g_menuAnimSpeed = Vars::menuAnimSpeed;

    BindSystem::get().process();

    nh::lua::Manager::get().update();
    nh::lua::Manager::get().dispatch(nh::lua::Event::Frame);
    nh::lua::Manager::get().dispatch(nh::lua::Event::Draw);

    nh::lua::closeImguiScopes();

    nh::blur::newFrame();

    DrawWatermark();
    BindSystem::get().drawBindsOverlay();
    BindSystem::get().drawBindPopup();
    BindSystem::get().drawHotkeysList();
    DrawAboutWindow();

    DrawScriptConsoleWindow();

    {
        static bool s_prevMenuOpen = false;
        if (s_prevMenuOpen && !Vars::menuOpen && Vars::autoSave)
            Config::get().save();
        s_prevMenuOpen = Vars::menuOpen;
    }

    gui.m_fade = fi_lerp(gui.m_fade, Vars::menuOpen ? 1.f : 0.f, 0.40f);

    if (gui.m_fade < 0.004f) {
        gui.m_anim = 0.f;
        return;
    }

    PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

    const bool  blurOn = Vars::menuOpen && Vars::guiBlur && nh::blur::available();
    const float bgMul  = blurOn ? 0.78f : 1.f;

    gui.group_box_bg.a  = blurOn ? 0.50f : 1.f;
    gui.frame_inactive.a = blurOn ? 0.88f : 1.f;
    gui.button_bg.a     = blurOn ? 0.80f : 1.f;

    PushStyleVar(ImGuiStyleVar_Alpha, gui.m_fade);
    SetNextWindowBgAlpha(gui.m_fade * bgMul);

    SetNextWindowPos(ImVec2(100.f, 100.f), ImGuiCond_FirstUseEver);

    ImGui::Begin("##Neverhook", NULL, ImGuiWindowFlags_NoDecoration);
    {
        if (ImGui::IsKeyPressed(ImGuiKey_Escape, false) || 
            ImGui::IsKeyPressed(ImGuiKey_Insert, false) || 
            (ImGui::IsKeyPressed(ImGuiKey_Tab, false) && !ImGui::GetIO().WantTextInput)) {
            Vars::menuOpen = false;
            ImGui::ClearActiveID();
            ImGui::SetWindowFocus(nullptr);
        }

        auto window = GetCurrentWindow();
        auto draw = window->DrawList;
        auto pos = window->Pos;
        auto size = window->Size;
        auto style = GetStyle(); (void)style;
        ImGuiIO& io = GetIO();

        gui.m_anim = fi_lerp(gui.m_anim, 1.f, 0.15f);

        SetWindowSize(ImVec2(690, 500));

        if (blurOn)
            nh::blur::submit(GetBackgroundDrawList(), pos, pos + size, 6.f,
                             Vars::guiBlurStrength, gui.m_fade);

        draw->AddRectFilled(
            pos, pos + ImVec2(170.f, size.y),
            ImColor(0.022f, 0.038f, 0.072f, gui.m_fade * bgMul),
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
        if (gui.tab(ICON_FA_CROSSHAIRS, "Player", gui.m_tab == 0 && g_featureSearch[0] == '\0')) {
            g_featureSearch[0] = '\0'; ImGui::ClearActiveID(); gui.m_tab = 0; gui.m_anim = 0.f;
        }

        if (gui.tab(ICON_FA_GHOST, "Global", gui.m_tab == 1 && g_featureSearch[0] == '\0')) {
            g_featureSearch[0] = '\0'; ImGui::ClearActiveID(); gui.m_tab = 1; gui.m_anim = 0.f;
        }

        if (gui.tab(ICON_FA_MOUSE, "Macros", gui.m_tab == 2 && g_featureSearch[0] == '\0')) {
            g_featureSearch[0] = '\0'; ImGui::ClearActiveID(); gui.m_tab = 2; gui.m_anim = 0.f;
        }

        Spacing();

        gui.group_title("Visuals");
        if (gui.tab(ICON_FA_USER, "Cosmetic", gui.m_tab == 3 && g_featureSearch[0] == '\0')) {
            g_featureSearch[0] = '\0'; ImGui::ClearActiveID(); gui.m_tab = 3; gui.m_anim = 0.f;
        }

        if (gui.tab(ICON_FA_PALLET, "Bypass", gui.m_tab == 4 && g_featureSearch[0] == '\0')) {
            g_featureSearch[0] = '\0'; ImGui::ClearActiveID(); gui.m_tab = 4; gui.m_anim = 0.f;
        }

        Spacing();

        gui.group_title("Miscellaneous");
        if (gui.tab(ICON_FA_HAMMER, "Creator", gui.m_tab == 5 && g_featureSearch[0] == '\0')) {
            g_featureSearch[0] = '\0'; ImGui::ClearActiveID(); gui.m_tab = 5; gui.m_anim = 0.f;
        }

        if (gui.tab(ICON_FA_CODE, "Scripts", gui.m_tab == 6 && g_featureSearch[0] == '\0')) {
            g_featureSearch[0] = '\0'; ImGui::ClearActiveID(); gui.m_tab = 6; gui.m_anim = 0.f;
        }

        {
            auto& scriptTabs = nh::lua::Manager::get().tabs();

            if (!scriptTabs.empty()) {
                Spacing();
                gui.group_title("Scripts");

                for (int i = 0; i < (int)scriptTabs.size(); ++i) {
                    const int tabId = 100 + i;

                    ImGui::PushID(i);

                    if (gui.tab(ICON_FA_HOME, scriptTabs[i].title.c_str(), gui.m_tab == tabId && g_featureSearch[0] == '\0')) {
                        g_featureSearch[0] = '\0'; ImGui::ClearActiveID(); gui.m_tab = tabId; gui.m_anim = 0.f;
                    }

                    ImGui::PopID();
                }
            }

            if (gui.m_tab >= 100 && gui.m_tab - 100 >= (int)scriptTabs.size())
                gui.m_tab = 6, gui.m_anim = 0.f;
        }

        EndChild();

        SetCursorPos(ImVec2(190, 20));
        if (gui.button(ICON_FA_SAVE " Save", ImVec2(100, 25)))
            Config::get().save();

        if (gui.m_tab == 2) {
            PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

            SetCursorPos(ImVec2(300, 20));
            BeginChild("##subtabs", ImVec2(160, 25));

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
        }

        {
            const float searchX = 472.f;
            const float searchW = size.x - searchX - 15.f;
            const float gap = 6.f;

            PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.f, 5.5f));
            const float inputH = GetFrameHeight();
            const float btnSize = inputH;
            const float inputW = searchW - btnSize - gap;

            SetCursorPos(ImVec2(searchX, 20.f));
            BeginChild("##globalsearchbar", ImVec2(searchW, inputH + 2.f), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

            if (g_featureSearch[0] != '\0') {
                if (gui.button(ICON_FA_TIMES, ImVec2(btnSize, inputH))) {
                    g_featureSearch[0] = '\0';
                    ImGui::ClearActiveID();
                }
            } else {
                Dummy(ImVec2(btnSize, inputH));
            }

            SameLine(0.f, gap);

            PushItemWidth(inputW);
            InputTextWithHint("##featsearch", ICON_FA_SEARCH "  Search...", g_featureSearch, IM_ARRAYSIZE(g_featureSearch));
            PopItemWidth();

            EndChild();
            PopStyleVar();
        }

        PushStyleVar(ImGuiStyleVar_Alpha, gui.m_anim * gui.m_fade);
        PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
        PushStyleVar(ImGuiStyleVar_ScrollbarSize, 6.f);
        PushStyleVar(ImGuiStyleVar_ScrollbarRounding, 12.f);
        PushStyleColor(ImGuiCol_ScrollbarBg, ImVec4(0.f, 0.f, 0.f, 0.15f));
        PushStyleColor(ImGuiCol_ScrollbarGrab, gui.accent_color.to_vec4(0.65f));
        PushStyleColor(ImGuiCol_ScrollbarGrabHovered, gui.accent_color.to_vec4(0.85f));
        PushStyleColor(ImGuiCol_ScrollbarGrabActive, gui.accent_color.to_vec4(1.00f));

        if (g_featureSearch[0] != '\0' || gui.m_tab != 6 && gui.m_tab < 100) {
            SetCursorPos(ImVec2(185, 81 - (5 * gui.m_anim)));
            BeginChild("##childs", ImVec2(size.x - 200, size.y - 96), false, g_featureSearch[0] != '\0' ? ImGuiWindowFlags_NoScrollWithMouse : ImGuiWindowFlags_NoScrollbar);
        } else {
            SetCursorPos(ImVec2(185, 73 - (5 * gui.m_anim)));
            BeginChild("##childs", ImVec2(size.x - 200, size.y - 88), false, ImGuiWindowFlags_NoScrollbar);
        }

        if (g_featureSearch[0] != '\0') {
            DrawFeatureSearchResults(g_featureSearch);
        } else {
            switch (gui.m_tab) {

        case 0:

            gui.group_box(ICON_FA_USER " Player", ImVec2(GetWindowWidth(), GetWindowHeight())); {

                gui.toggle("Noclip", &Vars::noclip);
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

                gui.toggle("Show Hitboxes", &Vars::showHitboxes);
                gui.toggle("Show On Death", &Vars::showHitboxesOnDeath);
                gui.toggle("Show Trajectory (WIP)", &Vars::showTrajectory);
                gui.toggle("Click Between Frames", &Vars::clickBetweenFrames);

                gui.toggle("Hitbox Multiplier", &Vars::hitboxMultiplier);
                if (Vars::hitboxMultiplier) {
                    gui.slider_float("Player",  &Vars::hitboxMultPlayer, 0.1f, 3.0f, "%.1fx");
                    gui.slider_float("Solids",  &Vars::hitboxMultSolid,  0.1f, 3.0f, "%.1fx");
                    gui.slider_float("Hazards", &Vars::hitboxMultHazard, 0.1f, 3.0f, "%.1fx");
                }

                gui.toggle("Instant Complete", &Vars::instantComplete);
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

                gui.toggle("Jump Hack", &Vars::jumpHack);
                gui.toggle("All Modes Platformer", &Vars::allModesPlatformer);
                gui.toggle("Auto Practice Mode", &Vars::autoPracticeMode);
                gui.toggle("Auto Pickup Coins", &Vars::autoPickupCoins);
                gui.toggle("Auto Song Download", &Vars::autoSongDownload);
                gui.toggle("Layout Mode", &Vars::layoutMode);

                Spacing();
                gui.toggle("Speedhack", &Vars::speedhack);
                if (Vars::speedhack) {
                    PushItemWidth(-1);
                    InputFloat("##speed", &Vars::speedhackValue, 0.f, 0.f, "%.2fx");
                    PopItemWidth();
                }
                gui.toggle("Speedhack Audio", &Vars::speedhackAudio);

            } gui.end_group_box();

            break;

        case 1:

            gui.group_box(ICON_FA_INFO_CIRCLE " Status", ImVec2(GetWindowWidth() / 2 - GetStyle().ItemSpacing.x / 2, GetWindowHeight())); {

                gui.toggle("Hide Status", &Vars::hideStatus);
                Spacing();
                const char* const font_names[] = { "Big Font", "Chat Font", "Gold Font" };
                PushItemWidth(-1);
                Combo("##stfont", &Vars::statusFont, font_names, IM_ARRAYSIZE(font_names));
                PopItemWidth();
                gui.slider_float("Opacity", &Vars::statusOpacity, 0.f, 100.f, "%.0f%%");
                gui.slider_float("Scale", &Vars::statusScale, 0.1f, 2.0f, "%.2fx");
                Separator();

                const char* const pos_names[] = { "Off", "Top-Left", "Top-Right", "Bottom-Left", "Bottom-Right", "Top-Center", "Bottom-Center" };

                auto statusItem = [&](const char* label, const char* id, int* posVal) {
                    ImGui::PushID(id);
                    ImGui::TextUnformatted(label);
                    ImGui::SameLine(ImGui::GetWindowWidth() - 120.f);
                    ImGui::PushItemWidth(110.f);
                    ImGui::Combo("##pos", posVal, pos_names, IM_ARRAYSIZE(pos_names));
                    ImGui::PopItemWidth();
                    ImGui::PopID();
                };

                for (int i = 0; i < (int)Vars::statusOrder.size(); ++i) {
                    int id = Vars::statusOrder[i];
                    ImGui::PushID(id);

                    if (i > 0) {
                        if (gui.button(ICON_FA_ARROW_UP, ImVec2(15.f, 18.f))) {
                            std::swap(Vars::statusOrder[i], Vars::statusOrder[i - 1]);
                            ImGui::PopID();
                            break;
                        }
                    } else {
                        Dummy(ImVec2(15.f, 18.f));
                    }

                    SameLine(0.f, 1.f);

                    if (i < (int)Vars::statusOrder.size() - 1) {
                        if (gui.button(ICON_FA_ARROW_DOWN, ImVec2(15.f, 18.f))) {
                            std::swap(Vars::statusOrder[i], Vars::statusOrder[i + 1]);
                            ImGui::PopID();
                            break;
                        }
                    } else {
                        Dummy(ImVec2(15.f, 18.f));
                    }

                    SameLine(0.f, 4.f);

                    switch (id) {
                    case 0:
                        statusItem("Cheat Indicator", "ci", &Vars::statusCheatIndicator);
                        if (Vars::statusCheatIndicator > 0) {
                            const char* const mode_names[] = { "Dot", "Text" };
                            ImGui::PushID("ci_mode");
                            ImGui::TextUnformatted("Indicator Mode");
                            ImGui::SameLine(ImGui::GetWindowWidth() - 120.f);
                            ImGui::PushItemWidth(110.f);
                            ImGui::Combo("##mode", &Vars::statusCheatIndicatorMode, mode_names, IM_ARRAYSIZE(mode_names));
                            ImGui::PopItemWidth();
                            ImGui::PopID();
                        }
                        break;
                    case 1: statusItem("FPS", "fps", &Vars::statusFps); break;
                    case 2: statusItem("CPS", "cps", &Vars::statusCps); break;
                    case 3: statusItem("Best Run", "br", &Vars::statusBestRun); break;
                    case 4: statusItem("Noclip Acc", "ncacc", &Vars::statusNoclipAcc); break;
                    case 5: statusItem("Noclip Deaths", "ncd", &Vars::statusNoclipDeaths); break;
                    case 6: statusItem("Attempts", "att", &Vars::statusAttempts); break;
                    case 7: statusItem("Jumps", "jmp", &Vars::statusJumps); break;
                    case 8: statusItem("Percentage", "pct", &Vars::statusPercentage); break;
                    case 9: statusItem("Level Time", "lt", &Vars::statusLevelTime); break;
                    case 10: statusItem("Session Time", "st", &Vars::statusSessionTime); break;
                    case 11: statusItem("Clock", "clk", &Vars::statusClock); break;
                    case 12: statusItem("Frame Counter", "fc", &Vars::statusFrameCounter); break;
                    case 13: statusItem("Position", "pos", &Vars::statusPosition); break;
                    case 14: statusItem("Velocity", "vel", &Vars::statusVelocity); break;
                    case 15:
                        statusItem("Message", "msg", &Vars::statusMessage);
                        if (Vars::statusMessage > 0) {
                            static char msgBuf[128] = "";
                            static std::string lastLoadedStr = "";
                            if (lastLoadedStr != Vars::statusMessageText && !ImGui::IsItemActive()) {
                                std::strncpy(msgBuf, Vars::statusMessageText.c_str(), sizeof(msgBuf) - 1);
                                lastLoadedStr = Vars::statusMessageText;
                            }
                            PushItemWidth(-1);
                            if (InputText("##msgtext", msgBuf, sizeof(msgBuf))) {
                                Vars::statusMessageText = msgBuf;
                                lastLoadedStr = msgBuf;
                            }
                            PopItemWidth();
                        }
                        break;
                    case 16: statusItem("Testmode", "tm", &Vars::statusTestmode); break;
                    case 17: statusItem("Replay State", "rs", &Vars::statusReplayState); break;
                    }

                    ImGui::PopID();
                }

            } gui.end_group_box();

            SameLine();

            gui.group_box(ICON_FA_DESKTOP " Display", ImVec2(GetWindowWidth() / 2 - GetStyle().ItemSpacing.x / 2, GetWindowHeight())); {

                if (gui.toggle("FPS Bypass", &Vars::fpsUnlock))
                    ApplyFPS();

                if (Vars::fpsUnlock) {
                    PushItemWidth(-1);
                    if (InputFloat("##fps", &Vars::fpsValue, 0.f, 0.f, "%.0f FPS"))
                        ApplyFPS();
                    PopItemWidth();
                }

                Spacing();
                gui.toggle("TPS Bypass", &Vars::tpsBypass);
                if (Vars::tpsBypass) {
                    PushItemWidth(-1);
                    InputFloat("##tps", &Vars::tpsValue, 0.f, 0.f, "%.0f TPS");
                    PopItemWidth();
                }

                Spacing();
                gui.toggle("Frame Extrapolation", &Vars::frameExtrapolation);
                gui.toggle("Compact List", &Vars::compactList);

                Spacing();
                gui.toggle("Endscreen Stats", &Vars::endscreenStats);
                if (Vars::endscreenStats)
                    gui.toggle("Endscreen Phrases", &Vars::endscreenPhrases);
            } gui.end_group_box();

            break;

        case 2:

            gui.group_box(ICON_FA_FILM " Macros", ImVec2(GetWindowWidth() / 2 - GetStyle().ItemSpacing.x / 2, GetWindowHeight())); {

                auto& eng = nh::MacroEngine::get();

                static char nameBuf[128] = "";
                static bool nameInit = false;
                if (!nameInit) {
                    std::strncpy(nameBuf, eng.currentName.c_str(), sizeof(nameBuf) - 1);
                    nameBuf[sizeof(nameBuf) - 1] = '\0';
                    nameInit = true;
                }

                TextDisabled("Name");
                PushItemWidth(-1);
                if (InputText("##macroName", nameBuf, sizeof(nameBuf)))
                    eng.currentName = nameBuf;
                PopItemWidth();

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

        case 3:

            gui.group_box(ICON_FA_USER " Visuals", ImVec2(GetWindowWidth(), GetWindowHeight())); {

                gui.toggle("Hide Attempts", &Vars::hideAttempts);
                gui.toggle("No Glow", &Vars::noGlow);

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
                if (Vars::waveTrailSize)
                    gui.slider_float("Size", &Vars::waveTrailSizeValue, 0.1f, 5.0f, "%.1fx");
                gui.toggle("No Shader", &Vars::noShader);
                gui.toggle("No Camera Shake", &Vars::noCameraShake);
                gui.toggle("No End Shake", &Vars::noEndShake);
                gui.toggle("No Death Shake", &Vars::noDeathShake);

                gui.toggle("Hide Complete VFX", &Vars::hideLevelCompleteVfx);
                gui.toggle("No Music Fade Out", &Vars::noMusicFadeOut);

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

            break;

        case 4:

            gui.group_box(ICON_FA_KEY " Bypass", ImVec2(GetWindowWidth(), GetWindowHeight())); {

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

                Spacing();
                gui.toggle("No Mirror Portal", &Vars::noMirrorPortal);
                gui.toggle("Instant Restart", &Vars::instantRestart);
                gui.toggle("Custom Respawn", &Vars::customRespawn);
                if (Vars::customRespawn && !Vars::instantRestart) {
                    gui.slider_float("Respawn Time", &Vars::respawnTime, 0.05f, 5.0f, "%.2fs");
                }
                gui.toggle("Pause On Complete", &Vars::pauseDuringComplete);
                gui.toggle("Hide Pause Menu", &Vars::hidePauseMenu);
                const bool zoomModLoaded = geode::Loader::get()->isModLoaded("bobby_shmurner.zoom");
                gui.toggle("Mouse Zoom on Pause", &Vars::mouseZoomOnPause);
                if (zoomModLoaded) {
                    TextDisabled("Zoooom! mod is already active");
                }

            } gui.end_group_box();

            break;

        case 5:

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

        case 6:

            DrawScriptsTab();
            break;

        default:

            if (gui.m_tab >= 100) {
                if (auto* scriptTab = nh::lua::Manager::get().tabAt(gui.m_tab - 100))
                    DrawScriptMenuTab(*scriptTab);
            }
            break;
        }
        }

        if (g_featureSearch[0] != '\0') {
            auto inner = GetCurrentWindow();
            if (inner) {
                static float currentScroll = 0.f;
                static float targetScroll = 0.f;
                static bool initScroll = false;
                if (!initScroll) {
                    currentScroll = inner->Scroll.y;
                    targetScroll = inner->Scroll.y;
                    initScroll = true;
                }
                const float maxScroll = inner->ScrollMax.y;
                if (IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows)) {
                    const float wheel = GetIO().MouseWheel;
                    if (wheel != 0.0f) {
                        targetScroll = ImClamp(targetScroll - wheel * 45.f, 0.0f, maxScroll);
                    }
                } else {
                    targetScroll = ImClamp(targetScroll, 0.0f, maxScroll);
                }
                if (std::abs(inner->Scroll.y - currentScroll) > 2.0f && GetIO().MouseWheel == 0.0f) {
                    targetScroll = inner->Scroll.y;
                    currentScroll = inner->Scroll.y;
                } else if (maxScroll > 0.0f) {
                    float dt = ImMin(GetIO().DeltaTime, 0.05f);
                    if (dt <= 0.0f) dt = 1.0f / 60.0f;
                    const float speed = 12.0f;
                    const float factor = 1.0f - std::exp(-speed * dt);
                    currentScroll = ImLerp(currentScroll, targetScroll, factor);
                    if (std::abs(currentScroll - targetScroll) < 0.2f) {
                        currentScroll = targetScroll;
                    }
                    SetScrollY(currentScroll);
                }
            }
        }

        EndChild();

        PopStyleColor(4);
        PopStyleVar(5);
    }
    ImGui::End();

    PopStyleVar();
    PopStyleVar();
}
