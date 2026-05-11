#pragma once

// -----------------------------------------------------------------------------
// framework_widgets.h
//
// Ported from neverlose-last/examples/example_win32_directx9/gui.hpp
//
// FULL original framework: custom color_t palette, PushStyleColor / PushStyleVar
// usage, custom rounded tab and group_box rendering. Everything visual is
// preserved as the framework's author intended.
//
// Differences vs original:
//   * #pragma once header guard (was missing)
//   * No 'using namespace std;' at global scope (qualified std:: instead)
//   * IMGUI_DEFINE_MATH_OPERATORS guarded with #ifndef to avoid redefinition
// -----------------------------------------------------------------------------

#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif

#include "imgui.h"
#include "imgui_internal.h"

#include "color_t.hpp"

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>

// ---------------------------------------------------------------------------
// fi_lerp -- framerate-independent ImLerp.
//
// The original framework used `ImLerp(current, target, 0.05f)` inside per-frame
// code. That smoothing factor is applied once per frame, so the visual speed
// of the animation depends entirely on the current FPS (e.g. an FPS-unlock or
// fps-limit makes the GUI fade-ins look noticeably slower or faster).
//
// Reformulating with `1 - (1 - a)^(60 * dt)` keeps the *visual* speed identical
// to the original 60-FPS behaviour for any frame time. At dt = 1/60 the factor
// reduces to `a` (matches the old code exactly); at dt = 1/30 it doubles to
// roughly `2a - a^2`, compensating for the half-rate update.
// ---------------------------------------------------------------------------
static inline float fi_lerp( float current, float target, float alpha_60 ) {

    const float dt = ImGui::GetIO( ).DeltaTime;
    if ( dt <= 0.f )
        return current;

    const float a = 1.f - ImPow( 1.f - alpha_60, 60.f * dt );
    return ImLerp( current, target, a );
}

class c_gui {

public:

    float m_anim        = 0.f;
    float m_fade        = 0.f;  // 0 = fully hidden, 1 = fully visible (fade in/out)
    int   m_tab         = 0;

    int                              m_rage_subtab = 0;
    std::vector< const char* >       rage_subtabs  = { "General", "Anti aim", "Subtab" };

    color_t accent_color   = { 0.3f,  0.49f, 1.f,   1.f };

    color_t text           = { 1.f,   1.f,   1.f,   1.f };
    color_t text_disabled  = { 0.51f, 0.52f, 0.56f, 1.f };

    color_t border         = { 1.f,   1.f,   1.f,   0.03f };

    color_t frame_inactive = { 0.023f, 0.039f, 0.07f,  1.f };
    color_t frame_active   = { 0.043f, 0.07f,  0.137f, 1.f };

    // NOTE: renamed from `button` to `button_bg` to avoid colliding with the
    // new `bool button(...)` method below. Same RGBA values as the original.
    color_t button_bg      = { 0.031f, 0.035f, 0.058f, 1.f };
    color_t button_hovered = { 0.050f, 0.054f, 0.078f, 1.f };
    color_t button_active  = { 0.07f,  0.074f, 0.098f, 1.f };

    color_t group_box_bg   = { 0.019f, 0.035f, 0.062f, 1.f };

    void render_circle_for_horizontal_bar( ImVec2 pos, ImColor color, float alpha );

    inline void group_title( const char* name ) {

        ImGui::SetCursorPosX( ImGui::GetCursorPosX( ) + 5 );
        ImGui::PushStyleColor( ImGuiCol_Text, ImVec4( 1.f, 1.f, 1.f, 0.40f ) );
        ImGui::Text( "%s", name );
        ImGui::PopStyleColor( );
    }

    void group_box( const char* name, ImVec2 size_arg );
    void end_group_box( );

    bool tab( const char* icon, const char* label, bool selected );
    bool subtab( const char* label, bool selected, int size, ImDrawFlags flags );

    // -----------------------------------------------------------------------
    // Custom widgets (Neverlose-style).
    //
    // toggle        -- pill + white knob, replaces ImGui::Checkbox.
    // button        -- flat coloured rectangle with hover/press fade.
    // slider_float  -- thin track + accent fill + round white knob.
    // slider_int    -- same visuals as slider_float, integer storage.
    // -----------------------------------------------------------------------
    bool toggle      ( const char* label, bool*  v );
    bool button      ( const char* label, ImVec2 size_arg = ImVec2( 0, 0 ) );
    bool slider_float( const char* label, float* v, float v_min, float v_max, const char* format = "%.2f" );
    bool slider_int  ( const char* label, int*   v, int   v_min, int   v_max, const char* format = "%d"   );

};

inline c_gui gui;
