// -----------------------------------------------------------------------------
// framework_widgets.cpp
//
// 1:1 port of neverlose-last/examples/example_win32_directx9/gui.cpp.
//
// All custom colors (color_t), PushStyleColor / PushStyleVar calls and
// custom drawing logic are preserved. Only platform-/API-specific code
// (DX9) is absent here -- there was none in gui.cpp anyway.
//
// Adjustments needed for the newer ImGui 1.92.7 in Neverhook:
//   * BeginChild(name, size, bool, flags) was deprecated -- now passes
//     ImGuiChildFlags. ImGuiWindowFlags_AlwaysUseWindowPadding was moved
//     to ImGuiChildFlags_AlwaysUseWindowPadding.
// -----------------------------------------------------------------------------

#include "framework_widgets.h"

using namespace ImGui;

void c_gui::render_circle_for_horizontal_bar( ImVec2 pos, ImColor color, float alpha ) {

    auto draw = GetWindowDrawList( );
    draw->AddCircleFilled( pos, 6, ImColor( color.Value.x, color.Value.y, color.Value.z, alpha * GetStyle( ).Alpha ) );
}

bool c_gui::tab( const char* icon, const char* label, bool selected ) {

    auto window = GetCurrentWindow( );
    auto id = window->GetID( label );

    auto icon_size  = CalcTextSize( icon );        (void)icon_size;
    auto label_size = CalcTextSize( label, 0, 1 );

    auto pos = window->DC.CursorPos;
    auto draw = window->DrawList;

    ImRect bb( pos, pos + ImVec2( GetWindowWidth( ), 26 ) );
    ItemAdd( bb, id );
    ItemSize( bb, GetStyle( ).FramePadding.y );

    bool hovered, held;
    bool pressed = ButtonBehavior( bb, id, &hovered, &held );

    static std::unordered_map< ImGuiID, float > values;
    auto value = values.find( id );
    if ( value == values.end( ) ) {

        values.insert( { id, 0.f } );
        value = values.find( id );
    }

    value->second = fi_lerp( value->second, ( selected ? 1.f : 0.f ), 0.09f );

    draw->AddRectFilled( bb.Min, bb.Max, gui.accent_color.to_im_color( 0.32f * value->second ), 5 );

    // Left-edge accent stripe: fades in/out with the selection state
    if ( value->second > 0.005f ) {
        const float  bar_half_h = 8.f * value->second;
        const ImVec2 bar_min( bb.Min.x,        bb.GetCenter( ).y - bar_half_h );
        const ImVec2 bar_max( bb.Min.x + 3.f,  bb.GetCenter( ).y + bar_half_h );
        draw->AddRectFilled( bar_min, bar_max,
                             gui.accent_color.to_im_color( value->second ), 2.f );
    }

    draw->AddText( ImVec2( bb.Min.x + 10, bb.GetCenter( ).y - label_size.y / 2 ), gui.accent_color.to_im_color( ), icon );
    draw->AddText( ImVec2( bb.Min.x + 35, bb.GetCenter( ).y - label_size.y / 2 ), GetColorU32( ImGuiCol_Text ), label );

    return pressed;
}

bool c_gui::subtab( const char* label, bool selected, int size, ImDrawFlags flags ) {

    auto window = GetCurrentWindow( );
    auto id = window->GetID( label );

    auto label_size = CalcTextSize( label, 0, 1 );

    auto pos = window->DC.CursorPos;
    auto draw = window->DrawList;

    ImRect bb( pos, pos + ImVec2( GetWindowWidth( ) / size, GetWindowHeight( ) ) );
    ItemAdd( bb, id );
    ItemSize( bb, GetStyle( ).FramePadding.y );

    bool hovered, held;
    bool pressed = ButtonBehavior( bb, id, &hovered, &held );

    static std::unordered_map< ImGuiID, float > values;
    auto value = values.find( id );
    if ( value == values.end( ) ) {

        values.insert( { id, 0.f } );
        value = values.find( id );
    }

    value->second = fi_lerp( value->second, ( selected ? 1.f : 0.f ), 0.10f );

    draw->AddRectFilled( bb.Min, bb.Max, gui.accent_color.to_im_color( 0.50f * value->second ), 4, flags );

    draw->AddText( bb.GetCenter( ) - label_size / 2, selected ? gui.text.to_im_color( ) : gui.text_disabled.to_im_color( ), label );

    return pressed;
}

void c_gui::group_box( const char* name, ImVec2 size_arg ) {

    auto window = GetCurrentWindow( );
    auto pos = window->DC.CursorPos;

    BeginChild( std::string( name ).append( ".main" ).c_str( ), size_arg, ImGuiChildFlags_None, ImGuiWindowFlags_NoScrollbar );

    GetWindowDrawList( )->AddRectFilled( pos + ImVec2( 3, 24 ), pos + size_arg + ImVec2( 3, 3 ),
                                         ImColor( 0.f, 0.f, 0.f, 0.28f * gui.m_fade ), 6 );
    GetWindowDrawList( )->AddRectFilled( pos + ImVec2( 0, 20 ), pos + size_arg, gui.group_box_bg.to_im_color( ), 6 );
    GetWindowDrawList( )->AddRect( pos + ImVec2( 0, 20 ), pos + size_arg, gui.border.to_im_color( 2.5f ), 6 );
    // Top-edge inner highlight for perceived depth (thin bright line on top of panel)
    GetWindowDrawList( )->AddLine( pos + ImVec2( 7, 21 ), pos + ImVec2( size_arg.x - 7, 21 ),
                                   ImColor( 1.f, 1.f, 1.f, 0.055f * gui.m_fade ) );

    // Accent bullet bar to the left of the group title text
    {
        const auto  title_sz = CalcTextSize( name );
        const float bar_y0   = ( title_sz.y - 8.f ) * 0.5f;
        GetWindowDrawList( )->AddRectFilled( pos + ImVec2( 3.f, bar_y0 ),
                                            pos + ImVec2( 6.f, bar_y0 + 8.f ),
                                            gui.accent_color.to_im_color( 0.7f ) );
    }
    GetWindowDrawList( )->AddText( pos + ImVec2( 12, 0 ), GetColorU32( ImGuiCol_Text, 0.5f ), name );

    SetCursorPos( ImVec2( 12, 21 ) );
    PushStyleVar( ImGuiStyleVar_WindowPadding, { 0, 10 } );
    // ImGuiWindowFlags_AlwaysUseWindowPadding moved into ImGuiChildFlags in ImGui 1.90.
    BeginChild( name, { size_arg.x - 24, size_arg.y - 21 }, ImGuiChildFlags_AlwaysUseWindowPadding, ImGuiWindowFlags_NoScrollbar );

    BeginGroup( );

    PushStyleVar( ImGuiStyleVar_ItemSpacing, { 8, 7 } );
    PushStyleVar( ImGuiStyleVar_Alpha, gui.m_anim * gui.m_fade );  // multiply: Push replaces, not multiplies
}

void c_gui::end_group_box( ) {

    PopStyleVar( 3 );
    EndGroup( );
    EndChild( );
    EndChild( );
}

// -----------------------------------------------------------------------------
// toggle -- Neverlose-style pill replacement for Checkbox.
//
// Layout: [label                                                  [pill+knob]]
// The whole row is one clickable rect, so users can hit the label or the pill.
// All animations use fi_lerp -> framerate independent.
// -----------------------------------------------------------------------------
bool c_gui::toggle( const char* label, bool* v ) {

    auto window = GetCurrentWindow( );
    if ( window->SkipItems )
        return false;

    auto id    = window->GetID( label );
    auto draw  = window->DrawList;
    auto pos   = window->DC.CursorPos;

    const auto label_size = CalcTextSize( label, 0, true );

    const float pill_w = 30.f;  // +15% vs original
    const float pill_h = 16.f;  // +14% vs original
    const float row_w  = GetContentRegionAvail( ).x;
    const float row_h  = ImMax( label_size.y, pill_h );

    ImRect bb( pos, pos + ImVec2( row_w, row_h ) );
    ItemSize( bb, GetStyle( ).FramePadding.y );
    if ( !ItemAdd( bb, id ) )
        return false;

    bool hovered, held;
    bool pressed = ButtonBehavior( bb, id, &hovered, &held );
    if ( pressed )
        *v = !*v;

    // -- on/off transition ----------------------------------------------------
    static std::unordered_map< ImGuiID, float > anims;
    auto it = anims.find( id );
    if ( it == anims.end( ) )
        it = anims.insert( { id, *v ? 1.f : 0.f } ).first;
    it->second = fi_lerp( it->second, *v ? 1.f : 0.f, 0.18f );
    const float a = it->second;

    // -- pill (right-aligned) -------------------------------------------------
    const ImVec2 pill_max( bb.Max.x,            bb.GetCenter( ).y + pill_h * 0.5f );
    const ImVec2 pill_min( pill_max.x - pill_w, bb.GetCenter( ).y - pill_h * 0.5f );

    const ImVec4 off_v = ImColor( 0.10f, 0.13f, 0.24f, gui.m_fade ).Value;  // visible navy-gray when OFF
    const ImVec4 on_v  = gui.accent_color.to_im_color( ).Value;
    const ImVec4 col_v = ImLerp( off_v, on_v, a );
    draw->AddRectFilled( pill_min, pill_max, ImColor( col_v ), pill_h * 0.5f );

    // subtle hover ring
    if ( hovered ) {
        draw->AddRect( pill_min, pill_max,
                       ImColor( 1.f, 1.f, 1.f, 0.07f * gui.m_fade ), pill_h * 0.5f );
    }

    // -- white knob -----------------------------------------------------------
    const float knob_r = pill_h * 0.5f - 2.f;
    const float knob_x = ImLerp( pill_min.x + knob_r + 2.f,
                                 pill_max.x - knob_r - 2.f, a );
    const float knob_y = bb.GetCenter( ).y;
    draw->AddCircleFilled( ImVec2( knob_x, knob_y ), knob_r,
                           ImColor( 1.f, 1.f, 1.f, gui.m_fade ) );

    // -- label ----------------------------------------------------------------
    draw->AddText( ImVec2( pos.x, bb.GetCenter( ).y - label_size.y * 0.5f ),
                   GetColorU32( ImGuiCol_Text ), label );

    return pressed;
}

// -----------------------------------------------------------------------------
// button -- flat rectangle, fades from gui.button_bg -> gui.button_active on
// hover/press. Borders use gui.border for the same hairline look as group_box.
// -----------------------------------------------------------------------------
bool c_gui::button( const char* label, ImVec2 size_arg ) {

    auto window = GetCurrentWindow( );
    if ( window->SkipItems )
        return false;

    auto id   = window->GetID( label );
    auto draw = window->DrawList;
    auto pos  = window->DC.CursorPos;

    const auto label_size = CalcTextSize( label, 0, true );

    ImVec2 sz = size_arg;
    if ( sz.x <= 0.f ) sz.x = label_size.x + 16.f;
    if ( sz.y <= 0.f ) sz.y = label_size.y + 8.f;

    ImRect bb( pos, pos + sz );
    ItemSize( bb, GetStyle( ).FramePadding.y );
    if ( !ItemAdd( bb, id ) )
        return false;

    bool hovered, held;
    bool pressed = ButtonBehavior( bb, id, &hovered, &held );

    static std::unordered_map< ImGuiID, float > anims;
    auto it = anims.find( id );
    if ( it == anims.end( ) )
        it = anims.insert( { id, 0.f } ).first;
    const float target = held ? 1.f : ( hovered ? 0.55f : 0.f );
    it->second = fi_lerp( it->second, target, 0.16f );
    const float a = it->second;

    const ImVec4 base_v = gui.button_bg    .to_im_color( ).Value;
    const ImVec4 act_v  = gui.button_active.to_im_color( ).Value;
    const ImVec4 col_v  = ImLerp( base_v, act_v, a );

    draw->AddRectFilled( bb.Min, bb.Max, ImColor( col_v ),               4.f );
    draw->AddRect      ( bb.Min, bb.Max, gui.border.to_im_color( ),      4.f );

    draw->AddText( bb.GetCenter( ) - label_size * 0.5f,
                   GetColorU32( ImGuiCol_Text ), label );

    return pressed;
}

// -----------------------------------------------------------------------------
// _slider_scalar -- shared core for slider_float / slider_int.
//
// Layout (two-row item):
//   [ label                                                            value ]
//   [============================*-------------------------------------------]
// SliderBehavior handles all click + drag math; we only draw on top of it.
// -----------------------------------------------------------------------------
static bool _slider_scalar( const char* label, ImGuiDataType data_type, void* p_v,
                            const void* p_min, const void* p_max,
                            const char* format ) {

    using namespace ImGui;

    auto window = GetCurrentWindow( );
    if ( window->SkipItems )
        return false;

    const auto id   = window->GetID( label );
    const auto pos  = window->DC.CursorPos;
    const auto draw = window->DrawList;

    char value_buf[ 64 ];
    DataTypeFormatString( value_buf, IM_ARRAYSIZE( value_buf ), data_type, p_v, format );

    const auto label_size = CalcTextSize( label,     0, true );
    const auto value_size = CalcTextSize( value_buf, 0, false );

    const float row_w  = GetContentRegionAvail( ).x;
    const float text_h = ImMax( label_size.y, value_size.y );
    const float gap    = 4.f;
    const float bar_h  = 4.f;
    const float knob_r = 5.f;                    // > bar_h/2 so it pokes out

    // total bb has to extend by knob_r above and below the bar so the knob is
    // not clipped by ItemAdd's auto-cull.
    ImRect bb ( pos,
                pos + ImVec2( row_w, text_h + gap + bar_h + ( knob_r - bar_h * 0.5f ) ) );

    ItemSize( bb, GetStyle( ).FramePadding.y );
    if ( !ItemAdd( bb, id ) )
        return false;

    // Rect that the user actually drags (only the bar itself, not the labels).
    const ImRect bar( ImVec2( pos.x,    pos.y + text_h + gap ),
                      ImVec2( bb.Max.x, pos.y + text_h + gap + bar_h ) );

    // SliderBehavior() in modern ImGui only updates the value when g.ActiveId == id;
    // it does NOT activate the slider itself (only the higher-level SliderScalar()
    // does, via SetActiveID). We use ButtonBehavior() to perform that activation
    // on click/drag so the slider actually responds to mouse input.
    bool s_hovered, s_held;
    ButtonBehavior( bar, id, &s_hovered, &s_held );

    ImRect grab_bb;
    bool changed = SliderBehavior( bar, id, data_type, p_v, p_min, p_max, format,
                                   ImGuiSliderFlags_None, &grab_bb );
    if ( changed )
        DataTypeFormatString( value_buf, IM_ARRAYSIZE( value_buf ), data_type, p_v, format );

    // -- top text row ---------------------------------------------------------
    draw->AddText( pos,
                   GetColorU32( ImGuiCol_Text ), label );
    draw->AddText( ImVec2( bb.Max.x - value_size.x, pos.y ),
                   GetColorU32( ImGuiCol_TextDisabled ), value_buf );

    // -- compute fill ratio from raw value (edge-to-edge) ---------------------
    float t = 0.f;
    if ( data_type == ImGuiDataType_Float ) {
        const float vf   = *(const float*)p_v;
        const float vmin = *(const float*)p_min;
        const float vmax = *(const float*)p_max;
        if ( vmax != vmin )
            t = ImClamp( ( vf - vmin ) / ( vmax - vmin ), 0.f, 1.f );
    } else if ( data_type == ImGuiDataType_S32 ) {
        const int vi   = *(const int*)p_v;
        const int vmin = *(const int*)p_min;
        const int vmax = *(const int*)p_max;
        if ( vmax != vmin )
            t = ImClamp( (float)( vi - vmin ) / (float)( vmax - vmin ), 0.f, 1.f );
    }
    const float fx = ImLerp( bar.Min.x, bar.Max.x, t );

    // -- bar background + filled portion + knob -------------------------------
    draw->AddRectFilled( bar.Min, bar.Max,                  gui.frame_inactive.to_im_color( ), bar_h * 0.5f );
    draw->AddRectFilled( bar.Min, ImVec2( fx, bar.Max.y ),  gui.accent_color  .to_im_color( ), bar_h * 0.5f );

    draw->AddCircleFilled( ImVec2( fx, bar.GetCenter( ).y ), knob_r,
                           ImColor( 1.f, 1.f, 1.f, gui.m_fade ) );

    return changed;
}

bool c_gui::slider_float( const char* label, float* v, float v_min, float v_max, const char* format ) {
    return _slider_scalar( label, ImGuiDataType_Float, v, &v_min, &v_max, format );
}

bool c_gui::slider_int( const char* label, int* v, int v_min, int v_max, const char* format ) {
    return _slider_scalar( label, ImGuiDataType_S32, v, &v_min, &v_max, format );
}
