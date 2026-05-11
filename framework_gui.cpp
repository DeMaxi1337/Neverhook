// -----------------------------------------------------------------------------
// framework_gui.cpp
//
// Ported from neverlose-last/examples/example_win32_directx9/main.cpp.
//
// What was removed (platform-/render-API specific, would not compile or
// would crash inside Neverhook's OpenGL DLL):
//   * int main / WinMain / CreateWindowW / message pump
//   * Direct3DCreate9 / IDirect3DDevice9 / D3DPRESENT_PARAMETERS
//   * ImGui_ImplDX9_* (NewFrame / RenderDrawData / Init / Shutdown)
//   * ResetDevice / D3DERR_DEVICELOST handling
//   * D3DXCreateTextureFromFileInMemoryEx(avatar)  -- aval pic skipped
//   * D3DXCreateTextureFromFileExA(bg)             -- background skipped
//   * blur::draw_blur(...) wiring                  -- phase 2
//
// What is preserved 1:1 from the original framework:
//   * All PushStyleVar / PushStyleColor / PopStyleVar / PopStyleColor
//   * Custom palette through gui.frame_active / gui.accent_color / etc.
//   * Custom fonts (museo500 + Font Awesome merged, museo900 28px header)
//   * Layout, sizes, cursor positions, animations.
// -----------------------------------------------------------------------------

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

#include <string>
#include <vector>

using namespace ImGui;

// ---------------------------------------------------------------------------
// FrameWorkInit -- called ONCE from main.cpp right after the ImGui backend
// is initialised, before the very first NewFrame(). Loads the three custom
// fonts straight out of bytes.hpp.
//
// bytes.hpp defines `unsigned char museo500_binary[ ] = { ... };` (a plain
// global array, NOT a heap allocation), so we MUST tell ImGui that we keep
// ownership of the memory by setting FontDataOwnedByAtlas = false --
// otherwise the atlas would try to free a static array on shutdown.
// ---------------------------------------------------------------------------
void FrameWorkInit()
{
    ImGuiIO& io = GetIO();

    // ---------------------------------------------------------------------
    // Global style. The framework is mostly custom-drawn (gui.toggle, ...)
    // but a handful of paths still go through stock ImGui (ColorEdit popup,
    // Combo dropdown, scrollbars, etc.). Setting the style colours here
    // means those fallbacks blend in instead of looking like vanilla.
    //
    // Hierarchy (darkest to lightest navy) intentionally mirrors the
    // color_t palette in framework_widgets.h:
    //   WindowBg        -> deepest, the menu backdrop
    //   group_box_bg    -> one notch lighter, the panel inside the menu
    //   frame_inactive  -> slider track / disabled rows
    //   frame_active    -> selected tab / hovered rows
    // ---------------------------------------------------------------------
    ImGuiStyle& s = GetStyle( );

    // Geometry / rounding
    s.WindowRounding         = 6.f;
    s.ChildRounding          = 6.f;
    s.FrameRounding          = 4.f;
    s.PopupRounding          = 4.f;
    s.GrabRounding           = 4.f;
    s.TabRounding            = 4.f;
    s.ScrollbarRounding      = 4.f;

    s.WindowBorderSize       = 1.f;
    s.ChildBorderSize        = 0.f;
    s.FrameBorderSize        = 0.f;
    s.PopupBorderSize        = 1.f;

    s.WindowPadding          = ImVec2( 8.f, 8.f );
    s.FramePadding           = ImVec2( 6.f, 4.f );
    s.ItemSpacing            = ImVec2( 8.f, 8.f );
    s.ItemInnerSpacing       = ImVec2( 6.f, 4.f );
    s.IndentSpacing          = 18.f;
    s.ScrollbarSize          = 8.f;
    s.GrabMinSize            = 10.f;

    // Colours (using the framework palette where it matches)
    ImVec4* c = s.Colors;
    c[ ImGuiCol_WindowBg            ] = ImVec4( 0.012f, 0.020f, 0.045f, 1.f ); // deepest
    c[ ImGuiCol_ChildBg             ] = ImVec4( 0.f,    0.f,    0.f,    0.f ); // group_box paints its own
    c[ ImGuiCol_PopupBg             ] = ImVec4( 0.019f, 0.035f, 0.062f, 0.98f );

    c[ ImGuiCol_Border              ] = ImVec4( 1.f, 1.f, 1.f, 0.05f );
    c[ ImGuiCol_BorderShadow        ] = ImVec4( 0.f, 0.f, 0.f, 0.f );

    c[ ImGuiCol_FrameBg             ] = gui.frame_inactive.to_vec4( );
    c[ ImGuiCol_FrameBgHovered      ] = gui.frame_active  .to_vec4( );
    c[ ImGuiCol_FrameBgActive       ] = gui.frame_active  .to_vec4( );

    c[ ImGuiCol_Text                ] = gui.text         .to_vec4( );
    c[ ImGuiCol_TextDisabled        ] = gui.text_disabled.to_vec4( );

    c[ ImGuiCol_Button              ] = gui.button_bg     .to_vec4( );
    c[ ImGuiCol_ButtonHovered       ] = gui.button_hovered.to_vec4( );
    c[ ImGuiCol_ButtonActive        ] = gui.button_active .to_vec4( );

    c[ ImGuiCol_Header              ] = gui.frame_active.to_vec4( );
    c[ ImGuiCol_HeaderHovered       ] = gui.frame_active.to_vec4( );
    c[ ImGuiCol_HeaderActive        ] = gui.frame_active.to_vec4( );

    c[ ImGuiCol_CheckMark           ] = gui.accent_color.to_vec4( );
    c[ ImGuiCol_SliderGrab          ] = gui.accent_color.to_vec4( );
    c[ ImGuiCol_SliderGrabActive    ] = gui.accent_color.to_vec4( );

    c[ ImGuiCol_ScrollbarBg         ] = ImVec4( 0.f, 0.f, 0.f, 0.f );
    c[ ImGuiCol_ScrollbarGrab       ] = ImVec4( 1.f, 1.f, 1.f, 0.10f );
    c[ ImGuiCol_ScrollbarGrabHovered] = ImVec4( 1.f, 1.f, 1.f, 0.15f );
    c[ ImGuiCol_ScrollbarGrabActive ] = ImVec4( 1.f, 1.f, 1.f, 0.20f );

    c[ ImGuiCol_Separator           ] = ImVec4( 1.f, 1.f, 1.f, 0.05f );
    c[ ImGuiCol_SeparatorHovered    ] = ImVec4( 1.f, 1.f, 1.f, 0.10f );
    c[ ImGuiCol_SeparatorActive     ] = gui.accent_color.to_vec4( );

    // ---------------------------------------------------------------------
    // Fonts
    // ---------------------------------------------------------------------

    // -- Fonts[0]: regular UI font (museo500) -------------------------------
    ImFontConfig cfg_regular;
    cfg_regular.FontDataOwnedByAtlas = false;
    io.Fonts->AddFontFromMemoryTTF( museo500_binary, (int)sizeof( museo500_binary ), 14.f, &cfg_regular );

    // -- Merged into Fonts[0]: Font Awesome icons ---------------------------
    // MergeMode = true means this font does NOT get its own slot in Fonts[];
    // its glyphs are merged into the previous font (museo500). That is why
    // io.Fonts->Fonts[1] below refers to museo900, not Font Awesome.
    static const ImWchar icon_ranges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
    ImFontConfig cfg_icons;
    cfg_icons.FontDataOwnedByAtlas = false;
    cfg_icons.MergeMode            = true;
    cfg_icons.PixelSnapH           = true;
    io.Fonts->AddFontFromMemoryTTF( font_awesome_binary, (int)sizeof( font_awesome_binary ), 13.f, &cfg_icons, icon_ranges );

    // -- Fonts[1]: big header font (museo900 28px) used for "NEVERLOSE" -----
    ImFontConfig cfg_big;
    cfg_big.FontDataOwnedByAtlas = false;
    io.Fonts->AddFontFromMemoryTTF( museo900_binary, (int)sizeof( museo900_binary ), 28.f, &cfg_big );
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
    // -- Fade in / out ---------------------------------------------------------
    // m_fade tracks 0..1 independently of menuOpen so we can still render
    // (and block input) during the closing animation.
    gui.m_fade = fi_lerp( gui.m_fade, Vars::menuOpen ? 1.f : 0.f, 0.40f );

    // Nothing to draw and no input to steal once fully faded out.
    if ( gui.m_fade < 0.004f ) {
        gui.m_anim = 0.f;   // reset per-tab anim so next open re-plays the slide-in
        return;
    }

    PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2( 0, 0 ) );
    // IMPORTANT: push Alpha BEFORE Begin() so the window border / bg / shadow
    // (drawn inside Begin() via GetColorU32(ImGuiCol_Border) which multiplies by
    // style.Alpha) also fades. Pushing it after Begin leaves a visible 1-px
    // outline ghost while the rest of the menu is fading out.
    PushStyleVar( ImGuiStyleVar_Alpha, gui.m_fade );
    SetNextWindowBgAlpha( gui.m_fade );  // backdrop alpha

    // First launch: place the menu in a sensible default position.
    // After first use, ImGui persists the user-dragged position via imgui.ini.
    SetNextWindowPos( ImVec2( 100.f, 100.f ), ImGuiCond_FirstUseEver );

    ImGui::Begin( "##Neverhook", NULL, ImGuiWindowFlags_NoDecoration );
    {
        auto window = GetCurrentWindow( );
        auto draw   = window->DrawList;
        auto pos    = window->Pos;
        auto size   = window->Size;
        auto style  = GetStyle( ); (void)style;
        ImGuiIO& io = GetIO( );

        gui.m_anim = fi_lerp( gui.m_anim, 1.f, 0.15f );

        SetWindowSize( ImVec2( 690, 500 ) );

        // Note: gui.m_fade is already applied via the outer PushStyleVar(Alpha)
        // before Begin(), so all to_im_color() calls in here automatically fade.
        // Raw ImColor() calls multiply by gui.m_fade explicitly below.

        // -- Left sidebar: solid fill, rounded to match WindowRounding so the
        //    rect doesn't poke out of the window's rounded corners.
        draw->AddRectFilled(
            pos, pos + ImVec2( 170.f, size.y ),
            ImColor( 0.022f, 0.038f, 0.072f, gui.m_fade ),
            6.f, ImDrawFlags_RoundCornersLeft
        );
        // Hairline divider between sidebar and content (line is 1 px wide so
        // it doesn't extend into the rounded corners)
        draw->AddLine( pos + ImVec2( 170.f, 6.f ), pos + ImVec2( 170.f, size.y - 6.f ),
                       gui.border.to_im_color( ) );

        // -- Horizontal divider below header / button row (kept clear of the
        //    right-side rounded corner)
        draw->AddLine( pos + ImVec2( 170.f, 60.f ), pos + ImVec2( size.x - 6.f, 60.f ),
                       gui.border.to_im_color( ) );

        // -- Static background image (DX9-only in original) removed:
        // GetBackgroundDrawList( )->AddImage( bg, ImVec2( 0, 0 ), io.DisplaySize );

        // -- "NEVERLOSE" header (museo900, 28 px).
        // io.Fonts->Fonts[1] is the big font because font_awesome was merged
        // into Fonts[0] (MergeMode = true).
        // ImGui 1.92 removed ImFont::FontSize (fonts are now multi-size) so
        // we pass the desired pixel size explicitly to CalcTextSizeA/AddText.
        if ( io.Fonts->Fonts.Size > 1 ) {
            ImFont*      big      = io.Fonts->Fonts[ 1 ];
            const float  big_size = 28.f;
            const ImVec2 nl_size  = big->CalcTextSizeA( big_size, FLT_MAX, 0, "NEVERHOOK" );
            draw->AddText( big, big_size, pos + ImVec2( 170 / 2 - nl_size.x / 2 + 1, 20 ), gui.accent_color.to_im_color( ), "NEVERHOOK" );
            draw->AddText( big, big_size, pos + ImVec2( 170 / 2 - nl_size.x / 2,     20 ), GetColorU32( ImGuiCol_Text ), "NEVERHOOK" );
        }

        // -- Footer: separator + avatar placeholder + user info
        draw->AddLine( pos + ImVec2( 8, size.y - 50 ), pos + ImVec2( 162, size.y - 50 ),
                       gui.border.to_im_color( ) );

        // Avatar: dark circle with accent ring + first initial
        {
            const float  av_cx = pos.x + 30.f;
            const float  av_cy = pos.y + size.y - 28.f;
            const float  av_r  = 14.f;

            draw->AddCircleFilled( ImVec2( av_cx, av_cy ), av_r,
                                   ImColor( 0.07f, 0.09f, 0.16f, gui.m_fade ) );
            draw->AddCircle      ( ImVec2( av_cx, av_cy ), av_r,
                                   gui.accent_color.to_im_color( 0.45f ) );

            const char init_str[ 2 ] = { 'D', '\0' };
            const auto init_sz = CalcTextSize( init_str );
            draw->AddText( ImVec2( av_cx - init_sz.x * 0.5f, av_cy - init_sz.y * 0.5f ),
                           GetColorU32( ImGuiCol_Text ), init_str );
        }

        draw->AddText( pos + ImVec2( 50, size.y - 40 ), gui.text         .to_im_color( ), "demaxihvh" );
        draw->AddText( pos + ImVec2( 50, size.y - 25 ), gui.text_disabled.to_im_color( ), "Till: " );
        draw->AddText( pos + ImVec2( 50 + CalcTextSize( "Till: " ).x, size.y - 25 ),
                       gui.accent_color.to_im_color( ), "Lifetime" );

        // ---------------------------------------------------------------
        //  Left side - tab list
        // ---------------------------------------------------------------
        SetCursorPos( ImVec2( 10, 70 ) );
        BeginChild( "##tabs", ImVec2( 150, size.y - 120 ) );

        gui.group_title( "Aimbot" );
        if ( gui.tab( ICON_FA_CROSSHAIRS, "Player", gui.m_tab == 0 ) && gui.m_tab != 0 )
            gui.m_tab = 0, gui.m_anim = 0.f;

        if ( gui.tab( ICON_FA_GHOST, "Global", gui.m_tab == 1 ) && gui.m_tab != 1 )
            gui.m_tab = 1, gui.m_anim = 0.f;

        if ( gui.tab( ICON_FA_MOUSE, "Macros", gui.m_tab == 2 ) && gui.m_tab != 2 )
            gui.m_tab = 2, gui.m_anim = 0.f;

        Spacing( );

        gui.group_title( "Visuals" );
        if ( gui.tab( ICON_FA_USER, "Cosmetic", gui.m_tab == 3 ) && gui.m_tab != 3 )
            gui.m_tab = 3, gui.m_anim = 0.f;

        if ( gui.tab( ICON_FA_PALLET, "Bypass", gui.m_tab == 4 ) && gui.m_tab != 4 )
            gui.m_tab = 4, gui.m_anim = 0.f;

        Spacing( );

        gui.group_title( "Miscellaneous" );
        if ( gui.tab( ICON_FA_HAMMER, "Creator", gui.m_tab == 5 ) && gui.m_tab != 5 )
            gui.m_tab = 5, gui.m_anim = 0.f;

        if ( gui.tab( ICON_FA_CODE, "Scripts", gui.m_tab == 6 ) && gui.m_tab != 6 )
            gui.m_tab = 6, gui.m_anim = 0.f;

        EndChild( );

        // ---------------------------------------------------------------
        //  Top bar - Save button + subtabs
        // ---------------------------------------------------------------
        SetCursorPos( ImVec2( 190, 20 ) );
        gui.button( ICON_FA_SAVE " Save", ImVec2( 100, 25 ) );

        PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2( 0, 0 ) );

        SetCursorPos( ImVec2( 300, 20 ) );
        BeginChild( "##subtabs", ImVec2( 240, 25 ) );

        GetWindowDrawList( )->AddRectFilled( GetWindowPos( ), GetWindowPos( ) + GetWindowSize( ), gui.button_bg.to_im_color( ), 4 );
        GetWindowDrawList( )->AddRect      ( GetWindowPos( ), GetWindowPos( ) + GetWindowSize( ), gui.border   .to_im_color( ), 4 );

        for ( int i = 0; i < (int)gui.rage_subtabs.size( ); ++i ) {

            ImDrawFlags flags = 0;
            if ( i == 0 )                                              flags = ImDrawFlags_RoundCornersLeft;
            else if ( i == (int)gui.rage_subtabs.size( ) - 1 )         flags = ImDrawFlags_RoundCornersRight;

            if ( gui.subtab( gui.rage_subtabs.at( i ), gui.m_rage_subtab == i, (int)gui.rage_subtabs.size( ), flags ) && gui.m_rage_subtab != i )
                gui.m_rage_subtab = i, gui.m_anim = 0.f;

            if ( i != (int)gui.rage_subtabs.size( ) - 1 )
                SameLine( );
        }

        EndChild( );

        PopStyleVar( );

        // Multiply by m_fade so closing the menu also fades the content area
        // (ImGui Push replaces, it does NOT multiply, the previous alpha).
        PushStyleVar( ImGuiStyleVar_Alpha,       gui.m_anim * gui.m_fade );
        PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2( 8, 5 ) );

        // ---------------------------------------------------------------
        //  Main content area
        // ---------------------------------------------------------------
        SetCursorPos( ImVec2( 185, 81 - ( 5 * gui.m_anim ) ) );
        BeginChild( "##childs", ImVec2( size.x - 200, size.y - 96 ) );

        // Per-tab content. Migrated from old gui.cpp DrawNeverhookMenu --
        // each old CollapsingHeader section now lives inside its renamed tab.
        switch ( gui.m_tab ) {

        case 0:  // Player  (was Ragebot)

            gui.group_box( ICON_FA_USER " Player", ImVec2( GetWindowWidth( ), GetWindowHeight( ) ) ); {

                gui.toggle( "Noclip",           &Vars::noclip );
                gui.toggle( "No Death Effect",  &Vars::noDeathEffect );
                gui.toggle( "No Respawn Flash", &Vars::noRespawnFlash );

            } gui.end_group_box( );

            break;

        case 1:  // Global  (was Anti-aim)

            gui.group_box( ICON_FA_RUNNING " Speedhack", ImVec2( GetWindowWidth( ) / 2 - GetStyle( ).ItemSpacing.x / 2, GetWindowHeight( ) ) ); {

                gui.toggle( "Speedhack", &Vars::speedhack );
                if ( Vars::speedhack )
                    gui.slider_float( "Speed", &Vars::speedhackValue, 0.1f, 5.0f, "%.2fx" );

            } gui.end_group_box( );

            SameLine( );

            gui.group_box( ICON_FA_TACHOMETER " FPS Bypass", ImVec2( GetWindowWidth( ) / 2 - GetStyle( ).ItemSpacing.x / 2, GetWindowHeight( ) ) ); {

                if ( gui.toggle( "FPS Bypass", &Vars::fpsUnlock ) )
                    ApplyFPS( );

                if ( Vars::fpsUnlock ) {
                    if ( gui.slider_float( "FPS", &Vars::fpsValue, 30.0f, 1000.0f, "%.0f FPS" ) )
                        ApplyFPS( );
                }

            } gui.end_group_box( );

            break;

        case 4:  // Bypass  (was World)

            gui.group_box( ICON_FA_KEY " Bypass", ImVec2( GetWindowWidth( ), GetWindowHeight( ) ) ); {

                gui.toggle( "Practice Music", &Vars::practiceMusic );
                gui.toggle( "Icon Bypass",    &Vars::iconBypass );

            } gui.end_group_box( );

            break;

        case 2:  // Macros   -- placeholder, no widgets yet
        case 3:  // Cosmetic -- placeholder, no widgets yet
        case 5:  // Creator  -- placeholder, no widgets yet
        case 6:  // Scripts  -- placeholder, no widgets yet
        default:
            break;
        }

        EndChild( );

        PopStyleVar( 2 );
    }
    ImGui::End( );

    PopStyleVar( );  // m_fade alpha (matches Push before Begin)
    PopStyleVar( );  // WindowPadding
}
