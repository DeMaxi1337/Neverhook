#include "framework_widgets.h"
#include "binds.h"
#include <cmath>

using namespace ImGui;

namespace {
    struct SmoothScrollState {
        float current = 0.f;
        float target  = 0.f;
        bool  init    = false;
    };
    static std::unordered_map< ImGuiID, SmoothScrollState > s_smoothScrolls;
}

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

    GetWindowDrawList( )->AddLine( pos + ImVec2( 7, 21 ), pos + ImVec2( size_arg.x - 7, 21 ),
                                   ImColor( 1.f, 1.f, 1.f, 0.055f * gui.m_fade ) );

    {
        const auto  title_sz = CalcTextSize( name );
        const float bar_y0   = 2.f + ( title_sz.y - 8.f ) * 0.5f;
        GetWindowDrawList( )->AddRectFilled( pos + ImVec2( 3.f, bar_y0 ),
                                            pos + ImVec2( 6.f, bar_y0 + 8.f ),
                                            gui.accent_color.to_im_color( 0.7f ) );
    }
    GetWindowDrawList( )->AddText( pos + ImVec2( 12, 2.f ), GetColorU32( ImGuiCol_Text, 0.5f ), name );

    SetCursorPos( ImVec2( 12, 26 ) );
    PushStyleVar( ImGuiStyleVar_WindowPadding, { 0, 8 } );
    PushStyleVar( ImGuiStyleVar_ScrollbarSize, 6.f );
    PushStyleVar( ImGuiStyleVar_ScrollbarRounding, 12.f );
    PushStyleColor( ImGuiCol_ScrollbarBg, ImVec4( 0.f, 0.f, 0.f, 0.15f ) );
    PushStyleColor( ImGuiCol_ScrollbarGrab, gui.accent_color.to_vec4( 0.65f ) );
    PushStyleColor( ImGuiCol_ScrollbarGrabHovered, gui.accent_color.to_vec4( 0.85f ) );
    PushStyleColor( ImGuiCol_ScrollbarGrabActive, gui.accent_color.to_vec4( 1.00f ) );

    BeginChild( name, { size_arg.x - 24, size_arg.y - 32 }, ImGuiChildFlags_AlwaysUseWindowPadding, ImGuiWindowFlags_NoScrollWithMouse );

    BeginGroup( );

    PushStyleVar( ImGuiStyleVar_ItemSpacing, { 8, 7 } );
    PushStyleVar( ImGuiStyleVar_Alpha, gui.m_anim * gui.m_fade );
}

void c_gui::end_group_box( ) {

    PopStyleVar( 2 );
    EndGroup( );

    auto inner = GetCurrentWindow( );
    if ( inner ) {
        auto& state = s_smoothScrolls[ inner->ID ];
        if ( !state.init ) {
            state.current = inner->Scroll.y;
            state.target  = inner->Scroll.y;
            state.init    = true;
        }

        const float maxScroll = inner->ScrollMax.y;

        if ( IsWindowHovered( ImGuiHoveredFlags_RootAndChildWindows ) ) {
            const float wheel = GetIO( ).MouseWheel;
            if ( wheel != 0.0f ) {
                state.target = ImClamp( state.target - wheel * 45.f, 0.0f, maxScroll );
            }
        } else {
            state.target = ImClamp( state.target, 0.0f, maxScroll );
        }

        if ( std::abs( inner->Scroll.y - state.current ) > 2.0f && GetIO( ).MouseWheel == 0.0f ) {
            state.target  = inner->Scroll.y;
            state.current = inner->Scroll.y;
        } else if ( maxScroll > 0.0f ) {
            float dt = ImMin( GetIO( ).DeltaTime, 0.05f );
            if ( dt <= 0.0f ) dt = 1.0f / 60.0f;
            const float speed  = 12.0f;
            const float factor = 1.0f - std::exp( -speed * dt );
            state.current = ImLerp( state.current, state.target, factor );

            if ( std::abs( state.current - state.target ) < 0.2f ) {
                state.current = state.target;
            }
            SetScrollY( state.current );
        }
    }

    EndChild( );
    PopStyleColor( 4 );
    PopStyleVar( 3 );
    EndChild( );
}

bool c_gui::toggle( const char* label, bool* v ) {

    auto window = GetCurrentWindow( );
    if ( window->SkipItems )
        return false;

    auto id    = window->GetID( label );
    auto draw  = window->DrawList;
    auto pos   = window->DC.CursorPos;

    const auto label_size = CalcTextSize( label, 0, true );

    const float pill_w = 30.f;
    const float pill_h = 16.f;
    const float scroll_reserve = (window->ScrollMax.y > 0.0f || window->ScrollbarY) ? 0.f : 10.f;
    const float row_w  = GetContentRegionAvail( ).x - scroll_reserve;
    const float row_h  = ImMax( label_size.y, pill_h );

    ImRect bb( pos, pos + ImVec2( row_w, row_h ) );
    ItemSize( bb, GetStyle( ).FramePadding.y );
    if ( !ItemAdd( bb, id ) )
        return false;

    bool hovered, held;
    bool pressed = ButtonBehavior( bb, id, &hovered, &held );
    if ( pressed )
        *v = !*v;

    if ( hovered && BindSystem::get( ).rmbClicked( ) ) {
        BindSystem::get( ).registerBool( label, v );
        BindSystem::get( ).openPopup( label );
    }

    static std::unordered_map< ImGuiID, float > anims;
    auto it = anims.find( id );
    if ( it == anims.end( ) )
        it = anims.insert( { id, *v ? 1.f : 0.f } ).first;
    it->second = fi_lerp( it->second, *v ? 1.f : 0.f, 0.18f );
    const float a = it->second;

    const ImVec2 pill_max( bb.Max.x,            bb.GetCenter( ).y + pill_h * 0.5f );
    const ImVec2 pill_min( pill_max.x - pill_w, bb.GetCenter( ).y - pill_h * 0.5f );

    const ImVec4 off_v = ImColor( 0.10f, 0.13f, 0.24f, gui.m_fade ).Value;
    const ImVec4 on_v  = gui.accent_color.to_im_color( ).Value;
    const ImVec4 col_v = ImLerp( off_v, on_v, a );
    draw->AddRectFilled( pill_min, pill_max, ImColor( col_v ), pill_h * 0.5f );

    if ( hovered ) {
        draw->AddRect( pill_min, pill_max,
                       ImColor( 1.f, 1.f, 1.f, 0.07f * gui.m_fade ), pill_h * 0.5f );
    }

    const float knob_r = pill_h * 0.5f - 2.f;
    const float knob_x = ImLerp( pill_min.x + knob_r + 2.f,
                                 pill_max.x - knob_r - 2.f, a );
    const float knob_y = bb.GetCenter( ).y;
    draw->AddCircleFilled( ImVec2( knob_x, knob_y ), knob_r,
                           ImColor( 1.f, 1.f, 1.f, gui.m_fade ) );

    draw->AddText( ImVec2( pos.x, bb.GetCenter( ).y - label_size.y * 0.5f ),
                   GetColorU32( ImGuiCol_Text ), label );

    return pressed;
}

bool c_gui::button( const char* label, ImVec2 size_arg ) {

    auto window = GetCurrentWindow( );
    if ( window->SkipItems )
        return false;

    auto id   = window->GetID( label );
    auto draw = window->DrawList;
    auto pos  = window->DC.CursorPos;

    const char* label_end = FindRenderedTextEnd( label );
    const auto  label_size = CalcTextSize( label, label_end, true );

    ImVec2 sz = size_arg;
    if ( sz.x < 0.f )       sz.x = GetContentRegionAvail( ).x;
    else if ( sz.x == 0.f ) sz.x = label_size.x + 16.f;
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

    draw->AddText( ImVec2( bb.GetCenter( ).x - label_size.x * 0.5f,
                           bb.GetCenter( ).y - label_size.y * 0.5f + 1.f ),
                   GetColorU32( ImGuiCol_Text ), label, label_end );

    return pressed;
}

static bool _slider_scalar( const char* label, ImGuiDataType data_type, void* p_v,
                            const void* p_min, const void* p_max,
                            const char* format, float width ) {

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

    const float scroll_reserve = (window->ScrollMax.y > 0.0f || window->ScrollbarY) ? 0.f : 10.f;
    const float row_w  = ( width > 0.f ) ? width : (GetContentRegionAvail( ).x - scroll_reserve);
    const float text_h = ImMax( label_size.y, value_size.y );
    const float gap    = 4.f;
    const float bar_h  = 4.f;
    const float knob_r = 5.f;

    ImRect bb ( pos,
                pos + ImVec2( row_w, text_h + gap + bar_h + ( knob_r - bar_h * 0.5f ) ) );

    ItemSize( bb, GetStyle( ).FramePadding.y );
    if ( !ItemAdd( bb, id ) )
        return false;

    const ImRect bar( ImVec2( pos.x,    pos.y + text_h + gap ),
                      ImVec2( bb.Max.x, pos.y + text_h + gap + bar_h ) );

    const ImRect interact_bb( ImVec2( pos.x,    bar.Min.y - 6.f ),
                              ImVec2( bb.Max.x, bar.Max.y + 6.f ) );

    bool s_hovered, s_held;
    ButtonBehavior( interact_bb, id, &s_hovered, &s_held );

    ImRect grab_bb;
    bool changed = SliderBehavior( interact_bb, id, data_type, p_v, p_min, p_max, format,
                                   ImGuiSliderFlags_None, &grab_bb );
    if ( changed )
        DataTypeFormatString( value_buf, IM_ARRAYSIZE( value_buf ), data_type, p_v, format );

    if ( data_type == ImGuiDataType_Float
      && s_hovered && BindSystem::get( ).rmbClicked( ) ) {
        BindSystem::get( ).registerFloat( label, (float*)p_v,
                                          *(const float*)p_min, *(const float*)p_max );
        BindSystem::get( ).openPopup( label );
    }

    const char* label_display_end = FindRenderedTextEnd( label );
    draw->AddText( pos,
                   GetColorU32( ImGuiCol_Text ), label, label_display_end );

    static ImGuiID s_sl_edit_id    = 0;
    static bool    s_sl_edit_focus = false;

    if ( s_sl_edit_id == id ) {
        const float  edit_w        = ImMax( value_size.x + 14.f, 56.f );
        const ImVec2 layout_cursor = window->DC.CursorPos;
        SetCursorScreenPos( ImVec2( bb.Max.x - edit_w, pos.y - 3.f ) );
        SetNextItemWidth( edit_w );
        PushStyleColor( ImGuiCol_FrameBg, (ImU32)gui.frame_active.to_im_color( ) );
        char edit_lbl[ 32 ];
        snprintf( edit_lbl, sizeof( edit_lbl ), "##sledit%u", (unsigned)id );
        if ( s_sl_edit_focus ) { SetKeyboardFocusHere( ); s_sl_edit_focus = false; }
        bool commit = false;
        if ( data_type == ImGuiDataType_Float )
            commit = InputFloat( edit_lbl, (float*)p_v, 0.f, 0.f, format,
                                 ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll );
        else
            commit = InputInt( edit_lbl, (int*)p_v, 0, 0,
                               ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll );
        PopStyleColor( 1 );
        if ( commit || IsItemDeactivated( ) ) {
            if ( data_type == ImGuiDataType_Float )
                *(float*)p_v = ImClamp( *(float*)p_v, *(const float*)p_min, *(const float*)p_max );
            else
                *(int*)p_v = ImClamp( *(int*)p_v, *(const int*)p_min, *(const int*)p_max );
            s_sl_edit_id = 0;
            changed = true;
        }
        window->DC.CursorPos = layout_cursor;
    } else {
        draw->AddText( ImVec2( bb.Max.x - value_size.x, pos.y ),
                       GetColorU32( ImGuiCol_TextDisabled ), value_buf );
        const ImRect value_rect( ImVec2( bb.Max.x - value_size.x - 4.f, pos.y ),
                                 ImVec2( bb.Max.x, pos.y + text_h ) );
        if ( IsMouseHoveringRect( value_rect.Min, value_rect.Max ) && IsMouseClicked( 0 ) ) {
            s_sl_edit_id    = id;
            s_sl_edit_focus = true;
        }
    }

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

    draw->AddRectFilled( bar.Min, bar.Max,                  gui.frame_inactive.to_im_color( ), bar_h * 0.5f );
    draw->AddRectFilled( bar.Min, ImVec2( fx, bar.Max.y ),  gui.accent_color  .to_im_color( ), bar_h * 0.5f );

    draw->AddCircleFilled( ImVec2( fx, bar.GetCenter( ).y ), knob_r,
                           ImColor( 1.f, 1.f, 1.f, gui.m_fade ) );

    return changed;
}

bool c_gui::slider_float( const char* label, float* v, float v_min, float v_max, const char* format, float width ) {
    return _slider_scalar( label, ImGuiDataType_Float, v, &v_min, &v_max, format, width );
}

bool c_gui::slider_int( const char* label, int* v, int v_min, int v_max, const char* format ) {
    return _slider_scalar( label, ImGuiDataType_S32, v, &v_min, &v_max, format, 0.f );
}

static float _cell_anim( ImGuiID id, int slot, float target, float speed ) {

    static std::unordered_map< ImU32, float > anims;
    const ImU32 key = (ImU32)id ^ ( (ImU32)slot * 0x9E3779B9u );

    auto it = anims.find( key );
    if ( it == anims.end( ) )
        it = anims.insert( { key, target } ).first;

    it->second = fi_lerp( it->second, target, speed );
    return it->second;
}

static ImVec2 _cell_size( ImVec2 size_arg ) {

    ImVec2 sz = size_arg;
    if ( sz.x <= 0.f ) sz.x = ImGui::GetContentRegionAvail( ).x;
    if ( sz.y <= 0.f ) sz.y = 24.f;
    return sz;
}

bool c_gui::key_cell( const char* id_str, const char* text, bool capturing, ImVec2 size_arg ) {

    auto window = GetCurrentWindow( );
    if ( window->SkipItems )
        return false;

    const ImGuiID id   = window->GetID( id_str );
    const ImVec2  pos  = window->DC.CursorPos;
    auto          draw = window->DrawList;

    const ImVec2 sz = _cell_size( size_arg );
    ImRect bb( pos, pos + sz );
    ItemSize( bb, 0.f );
    if ( !ItemAdd( bb, id ) )
        return false;

    bool hovered, held;
    const bool pressed = ButtonBehavior( bb, id, &hovered, &held );

    const float a = _cell_anim( id, 0, held ? 1.f : ( hovered ? 0.55f : 0.f ), 0.16f );
    const float c = _cell_anim( id, 1, capturing ? 1.f : 0.f, 0.20f );

    ImVec4 col = ImLerp( gui.button_bg.to_im_color( ).Value,
                         gui.button_active.to_im_color( ).Value, a );

    if ( c > 0.005f ) {
        const float pulse = 0.30f + 0.22f * ImSin( (float)GetTime( ) * 6.5f );
        col = ImLerp( col, gui.accent_color.to_im_color( ).Value, pulse * c );
    }

    draw->AddRectFilled( bb.Min, bb.Max, ImColor( col ), 4.f );
    draw->AddRect( bb.Min, bb.Max,
                   c > 0.005f ? gui.accent_color.to_im_color( 0.55f * c )
                              : gui.border.to_im_color( 2.5f ), 4.f );

    const ImVec2 ts = CalcTextSize( text );
    draw->AddText( ImVec2( bb.GetCenter( ).x - ts.x * 0.5f, bb.GetCenter( ).y - ts.y * 0.5f ),
                   GetColorU32( ImGuiCol_Text ), text );

    return pressed;
}

bool c_gui::combo_cell( const char* id_str, int* v, const char* const* items, int count,
                        ImVec2 size_arg ) {

    auto window = GetCurrentWindow( );
    if ( window->SkipItems )
        return false;

    const ImGuiID id   = window->GetID( id_str );
    const ImVec2  pos  = window->DC.CursorPos;
    auto          draw = window->DrawList;

    const ImVec2 sz = _cell_size( size_arg );
    ImRect bb( pos, pos + sz );
    ItemSize( bb, 0.f );
    if ( !ItemAdd( bb, id ) )
        return false;

    bool hovered, held;
    const bool pressed = ButtonBehavior( bb, id, &hovered, &held );

    char popup_id[ 48 ];
    ImFormatString( popup_id, IM_ARRAYSIZE( popup_id ), "##cbc_%u", (unsigned)id );
    if ( pressed )
        OpenPopup( popup_id );

    const bool  open = IsPopupOpen( popup_id );
    const float a    = _cell_anim( id, 0, ( held || open ) ? 1.f : ( hovered ? 0.55f : 0.f ), 0.16f );
    const float o    = _cell_anim( id, 1, open ? 1.f : 0.f, 0.20f );

    const ImVec4 col = ImLerp( gui.button_bg.to_im_color( ).Value,
                               gui.button_active.to_im_color( ).Value, a );

    draw->AddRectFilled( bb.Min, bb.Max, ImColor( col ), 4.f );
    draw->AddRect( bb.Min, bb.Max, gui.border.to_im_color( 2.5f ), 4.f );

    if ( a > 0.005f ) {
        const float cx = bb.GetCenter( ).x;
        draw->AddRectFilled( ImVec2( ImLerp( cx, bb.Min.x + 5.f, a ), bb.Max.y - 2.f ),
                             ImVec2( ImLerp( cx, bb.Max.x - 5.f, a ), bb.Max.y - 0.7f ),
                             gui.accent_color.to_im_color( a ), 1.f );
    }

    const char*  cur = ( *v >= 0 && *v < count ) ? items[ *v ] : "";
    const ImVec2 ts  = CalcTextSize( cur );
    draw->AddText( ImVec2( bb.Min.x + 9.f, bb.GetCenter( ).y - ts.y * 0.5f ),
                   GetColorU32( ImGuiCol_Text ), cur );

    const ImVec2 cc( bb.Max.x - 12.f, bb.GetCenter( ).y );
    const float  ys = ImLerp( -1.8f,  1.8f, o );
    const float  yt = ImLerp(  2.8f, -2.8f, o );
    const ImU32  ac = (ImU32)ImColor( ImLerp( gui.text_disabled.to_im_color( ).Value,
                                              gui.accent_color.to_im_color( ).Value, a ) );
    draw->AddTriangleFilled( ImVec2( cc.x - 4.f, cc.y + ys ),
                             ImVec2( cc.x + 4.f, cc.y + ys ),
                             ImVec2( cc.x,       cc.y + yt ), ac );

    bool changed = false;
    SetNextWindowPos( ImVec2( bb.Min.x, bb.Max.y + 3.f ) );
    SetNextWindowSize( ImVec2( sz.x, 0.f ) );
    PushStyleColor( ImGuiCol_PopupBg,       (ImU32)ImColor( 0.020f, 0.035f, 0.062f, 0.98f ) );
    PushStyleColor( ImGuiCol_Border,        (ImU32)gui.border.to_im_color( 2.5f ) );
    PushStyleColor( ImGuiCol_Text,          (ImU32)gui.text.to_im_color( ) );
    PushStyleColor( ImGuiCol_Header,        (ImU32)gui.accent_color.to_im_color( 0.35f ) );
    PushStyleColor( ImGuiCol_HeaderHovered, (ImU32)gui.button_active.to_im_color( ) );
    PushStyleVar( ImGuiStyleVar_WindowPadding,  ImVec2( 5.f, 5.f ) );
    PushStyleVar( ImGuiStyleVar_WindowRounding, 4.f );
    if ( BeginPopup( popup_id ) ) {
        for ( int i = 0; i < count; ++i )
            if ( Selectable( items[ i ], i == *v ) ) { *v = i; changed = true; }
        EndPopup( );
    }
    PopStyleVar( 2 );
    PopStyleColor( 5 );

    return changed;
}

bool c_gui::slider_cell( const char* id_str, float* v, float v_min, float v_max,
                         ImVec2 size_arg, const char* format ) {

    auto window = GetCurrentWindow( );
    if ( window->SkipItems )
        return false;

    const ImGuiID id   = window->GetID( id_str );
    const ImVec2  pos  = window->DC.CursorPos;
    auto          draw = window->DrawList;

    const ImVec2 sz = _cell_size( size_arg );
    ImRect bb( pos, pos + sz );
    ItemSize( bb, 0.f );
    if ( !ItemAdd( bb, id ) )
        return false;

    char buf[ 64 ];
    ImFormatString( buf, IM_ARRAYSIZE( buf ), format, *v );
    ImVec2 ts = CalcTextSize( buf );

    const float gap    = 6.f;
    const float bar_h  = 4.f;
    const float knob_r = 5.f;
    const float pad_r  = 2.f;
    const float val_w  = ImMax( ts.x + pad_r, 30.f );

    const float cy     = bb.GetCenter( ).y;
    const float bar_x1 = ImMax( bb.Min.x + knob_r * 2.f, bb.Max.x - val_w - gap );
    const ImRect bar( ImVec2( bb.Min.x, cy - bar_h * 0.5f ),
                      ImVec2( bar_x1,   cy + bar_h * 0.5f ) );

    bool hovered, held;
    ButtonBehavior( bar, id, &hovered, &held );

    ImRect grab_bb;
    bool changed = SliderBehavior( bar, id, ImGuiDataType_Float, v, &v_min, &v_max,
                                   format, ImGuiSliderFlags_None, &grab_bb );
    if ( changed )
        ImFormatString( buf, IM_ARRAYSIZE( buf ), format, *v );

    static ImGuiID s_sc_edit_id    = 0;
    static bool    s_sc_edit_focus = false;

    const ImRect value_rect( ImVec2( bb.Max.x - val_w - gap, bb.Min.y ),
                             ImVec2( bb.Max.x,               bb.Max.y ) );

    if ( s_sc_edit_id == id ) {
        const float  edit_w        = ImMax( val_w + gap, 46.f );
        const ImVec2 layout_cursor = window->DC.CursorPos;
        SetCursorScreenPos( ImVec2( bb.Max.x - edit_w, cy - GetFontSize( ) * 0.5f - 3.f ) );
        SetNextItemWidth( edit_w );
        PushStyleColor( ImGuiCol_FrameBg, (ImU32)gui.frame_active.to_im_color( ) );
        PushStyleVar  ( ImGuiStyleVar_FramePadding, ImVec2( 2.f, 2.f ) );
        char edit_lbl[ 32 ];
        ImFormatString( edit_lbl, IM_ARRAYSIZE( edit_lbl ), "##sce%u", (unsigned)id );
        if ( s_sc_edit_focus ) { SetKeyboardFocusHere( ); s_sc_edit_focus = false; }
        const bool commit = InputFloat( edit_lbl, v, 0.f, 0.f, format,
                                        ImGuiInputTextFlags_EnterReturnsTrue
                                      | ImGuiInputTextFlags_AutoSelectAll );
        PopStyleVar  ( 1 );
        PopStyleColor( 1 );
        if ( commit || IsItemDeactivated( ) ) {
            *v           = ImClamp( *v, v_min, v_max );
            s_sc_edit_id = 0;
            changed      = true;
        }
        window->DC.CursorPos = layout_cursor;
    }

    float t = 0.f;
    if ( v_max != v_min )
        t = ImClamp( ( *v - v_min ) / ( v_max - v_min ), 0.f, 1.f );
    const float fx = ImLerp( bar.Min.x, bar.Max.x, t );

    draw->AddRectFilled( bar.Min, bar.Max,                 gui.frame_inactive.to_im_color( ), bar_h * 0.5f );
    draw->AddRectFilled( bar.Min, ImVec2( fx, bar.Max.y ), gui.accent_color  .to_im_color( ), bar_h * 0.5f );
    draw->AddCircleFilled( ImVec2( fx, cy ), knob_r, ImColor( 1.f, 1.f, 1.f, gui.m_fade ) );

    if ( s_sc_edit_id != id ) {
        const bool vhov = IsMouseHoveringRect( value_rect.Min, value_rect.Max );
        draw->AddText( ImVec2( bb.Max.x - ts.x - pad_r, cy - ts.y * 0.5f ),
                       vhov ? GetColorU32( ImGuiCol_Text )
                            : GetColorU32( ImGuiCol_TextDisabled ), buf );
        if ( vhov && IsMouseClicked( 0 ) ) {
            s_sc_edit_id    = id;
            s_sc_edit_focus = true;
        }
    }

    return changed;
}

bool c_gui::icon_x_cell( const char* id_str, ImVec2 size_arg ) {

    auto window = GetCurrentWindow( );
    if ( window->SkipItems )
        return false;

    const ImGuiID id   = window->GetID( id_str );
    const ImVec2  pos  = window->DC.CursorPos;
    auto          draw = window->DrawList;

    const ImVec2 sz = _cell_size( size_arg );
    ImRect bb( pos, pos + sz );
    ItemSize( bb, 0.f );
    if ( !ItemAdd( bb, id ) )
        return false;

    bool hovered, held;
    const bool pressed = ButtonBehavior( bb, id, &hovered, &held );

    const float a = _cell_anim( id, 0, hovered ? 1.f : 0.f, 0.18f );

    if ( a > 0.005f )
        draw->AddRectFilled( bb.Min, bb.Max, gui.accent_color.to_im_color( 0.18f * a ), 4.f );

    const ImVec2 cc = bb.GetCenter( );
    const float  r  = 3.4f + 0.6f * a;
    const ImU32  xc = (ImU32)ImColor( ImLerp( gui.text_disabled.to_im_color( ).Value,
                                              gui.accent_color.to_im_color( ).Value, a ) );
    draw->AddLine( cc + ImVec2( -r, -r ), cc + ImVec2( r, r ), xc, 1.5f );
    draw->AddLine( cc + ImVec2( -r,  r ), cc + ImVec2( r, -r ), xc, 1.5f );

    return pressed;
}
