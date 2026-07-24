#pragma once

#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif

#include "framework_widgets.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "hashes.hpp"
#include "vars.h"
#include "watermark.h"   // wm_combo() + WM_GEAR

#include <Geode/utils/web.hpp>

using namespace ImGui;

// Opened by the sidebar "Info" button (replaces the old user/license footer).
inline bool  g_aboutOpen = false;
inline float g_aboutAnim = 0.f;

// -----------------------------------------------------------------------------
// DrawAboutWindow -- Neverlose-style "About" panel. Drawn every frame from the
// always-on overlay section so it works independently of the main menu's fade.
// -----------------------------------------------------------------------------
inline void DrawAboutWindow()
{
    g_aboutAnim = fi_lerp( g_aboutAnim, g_aboutOpen ? 1.f : 0.f, 0.30f );
    if ( g_aboutAnim < 0.004f )
        return;

    ImGuiIO& io = GetIO();

    const ImVec2 win_sz( 300.f, 430.f );
    SetNextWindowPos( ImVec2( io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f ),
        ImGuiCond_Appearing, ImVec2( 0.5f, 0.5f ) );
    SetNextWindowSize( win_sz );

    PushStyleVar( ImGuiStyleVar_Alpha, g_aboutAnim );
    PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2( 0, 0 ) );
    SetNextWindowBgAlpha( g_aboutAnim );

    Begin( "##NeverhookAbout", nullptr,
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoResize );
    {
        ImGuiWindow* window = GetCurrentWindow();
        ImDrawList*  draw   = window->DrawList;
        const ImVec2 pos    = window->Pos;
        const ImVec2 size   = window->Size;

        // -- Background + hairline border
        draw->AddRectFilled( pos, pos + size,
            ImColor( 0.019f, 0.035f, 0.062f, g_aboutAnim ), 6.f );
        draw->AddRect( pos, pos + size, gui.border.to_im_color(), 6.f );

        // -- Title bar: gear icon + "About Neverhook" + close (X)
        const float bar_h = 34.f;
        draw->AddText( ImVec2( pos.x + 12.f, pos.y + 9.f ),
            gui.text_disabled.to_im_color(), WM_GEAR );
        draw->AddText( ImVec2( pos.x + 30.f, pos.y + 9.f ),
            gui.text.to_im_color(), "About Neverhook" );

        {
            const float  cb = 20.f;
            const ImVec2 cmin( pos.x + size.x - cb - 10.f, pos.y + 7.f );
            SetCursorScreenPos( cmin );
            if ( InvisibleButton( "##aboutclose", ImVec2( cb, cb ) ) )
                g_aboutOpen = false;
            const bool   hov = IsItemHovered();
            const ImU32  xc  = ( hov ? gui.text : gui.text_disabled ).to_im_color();
            const ImVec2 c   = cmin + ImVec2( cb * 0.5f, cb * 0.5f );
            const float  r   = 5.f;
            draw->AddLine( ImVec2( c.x - r, c.y - r ), ImVec2( c.x + r, c.y + r ), xc, 1.6f );
            draw->AddLine( ImVec2( c.x - r, c.y + r ), ImVec2( c.x + r, c.y - r ), xc, 1.6f );
        }

        draw->AddLine( ImVec2( pos.x, pos.y + bar_h ),
            ImVec2( pos.x + size.x, pos.y + bar_h ), gui.border.to_im_color() );

        // -- Big NEVERHOOK logo (same display font as the sidebar)
        float y = bar_h + 16.f;
        if ( io.Fonts->Fonts.Size > 1 )
        {
            ImFont*      big      = io.Fonts->Fonts[1];
            const float  big_size = 30.f;
            const ImVec2 nl       = big->CalcTextSizeA( big_size, FLT_MAX, 0, "NEVERHOOK" );
            draw->AddText( big, big_size, pos + ImVec2( size.x * 0.5f - nl.x * 0.5f + 1, y ),
                gui.accent_color.to_im_color(), "NEVERHOOK" );
            draw->AddText( big, big_size, pos + ImVec2( size.x * 0.5f - nl.x * 0.5f, y ),
                GetColorU32( ImGuiCol_Text ), "NEVERHOOK" );
            y += big_size + 14.f;
        }
        else
        {
            y += 24.f;
        }

        // -- Body (normal ImGui layout inside a padded child)
        SetCursorPos( ImVec2( 16.f, y ) );
        PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2( 8, 8 ) );
        BeginChild( "##aboutbody", ImVec2( size.x - 32.f, size.y - y - 14.f ) );

        Separator();
        Spacing();

        TextColored( gui.text_disabled.to_vec4(), "Author:" ); SameLine();
        TextColored( gui.text.to_vec4(),          "DeMaxi1337" );

        TextColored( gui.text_disabled.to_vec4(), "Branch:" ); SameLine();
        TextColored( gui.accent_color.to_vec4(),  "Beta" );

        TextColored( gui.text_disabled.to_vec4(), "Updated:" ); SameLine();
        TextColored( gui.text.to_vec4(),          "19.07.2026" );

        Spacing();
        Separator();
        Spacing();

        if ( gui.button( "Support me on Boosty", ImVec2( -1.f, 30.f ) ) )
            geode::utils::web::openLinkInBrowser( "https://boosty.to/demaxi1337" );

        Spacing();
        Separator();
        Spacing();

        gui.toggle( "Auto-Save", &Vars::autoSave );

        Spacing();
        Separator();
        Spacing();

        static const char* scales[] = { "Auto", "100%", "125%", "150%", "200%" };
        wm_combo( "Menu Scale", &Vars::menuScale, scales, IM_ARRAYSIZE( scales ) );

        Spacing();
        Separator();
        Spacing();

        gui.slider_float( "Animation Speed", &Vars::menuAnimSpeed, 0.25f, 3.0f, "%.2f" );

        EndChild();
        PopStyleVar();   // ItemSpacing
    }
    End();

    PopStyleVar( 2 );    // WindowPadding + Alpha
}
