#pragma once

#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif

#include "framework_widgets.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "hashes.hpp"
#include "vars.h"
#include <Geode/Geode.hpp>
#include <cmath>

#include <vector>
#include <string>
#include <unordered_map>
#include <algorithm>
#include <cstdio>

#ifdef _WIN32
extern "C" __declspec(dllimport) short __stdcall GetAsyncKeyState(int vKey);
#endif

using namespace ImGui;

enum class BindMode : int { AlwaysOn = 0, Toggle = 1, Hold = 2 };

static bool nh_vk_down( int vk ) {
#ifdef _WIN32
    return ( GetAsyncKeyState( vk ) & 0x8000 ) != 0;
#else
    (void)vk; return false;
#endif
}

static int nh_poll_pressed_vk( ) {
#ifdef _WIN32
    for ( int vk = 0x04; vk <= 0xFE; ++vk ) {
        if ( vk == 0x01 || vk == 0x02 || vk == 0x1B || vk == 0x2D ) continue;
        if ( ( GetAsyncKeyState( vk ) & 0x8000 ) != 0 ) return vk;
    }
#endif
    return 0;
}

static const char* nh_vk_name( int vk ) {
    static char buf[ 16 ];
    if ( vk == 0 ) return "None";
    if ( vk >= 'A' && vk <= 'Z' ) { buf[ 0 ] = (char)vk; buf[ 1 ] = 0; return buf; }
    if ( vk >= '0' && vk <= '9' ) { buf[ 0 ] = (char)vk; buf[ 1 ] = 0; return buf; }
    if ( vk >= 0x70 && vk <= 0x87 ) { snprintf( buf, sizeof( buf ), "F%d", vk - 0x6F ); return buf; }
    if ( vk >= 0x60 && vk <= 0x69 ) { snprintf( buf, sizeof( buf ), "Num%d", vk - 0x60 ); return buf; }
    switch ( vk ) {
    case 0x04: return "Mouse3";
    case 0x05: return "Mouse4";
    case 0x06: return "Mouse5";
    case 0x08: return "Backspace";
    case 0x09: return "Tab";
    case 0x0D: return "Enter";
    case 0x10: return "Shift";
    case 0x11: return "Ctrl";
    case 0x12: return "Alt";
    case 0x14: return "Caps";
    case 0x1B: return "Esc";
    case 0x20: return "Space";
    case 0x21: return "PageUp";
    case 0x22: return "PageDown";
    case 0x23: return "End";
    case 0x24: return "Home";
    case 0x25: return "Left";
    case 0x26: return "Up";
    case 0x27: return "Right";
    case 0x28: return "Down";
    case 0x2D: return "Insert";
    case 0x2E: return "Delete";
    case 0xA0: return "LShift";
    case 0xA1: return "RShift";
    case 0xA2: return "LCtrl";
    case 0xA3: return "RCtrl";
    case 0xA4: return "LAlt";
    case 0xA5: return "RAlt";
    case 0xBA: return ";";
    case 0xBB: return "=";
    case 0xBC: return ",";
    case 0xBD: return "-";
    case 0xBE: return ".";
    case 0xBF: return "/";
    case 0xC0: return "`";
    case 0xDB: return "[";
    case 0xDC: return "\\";
    case 0xDD: return "]";
    case 0xDE: return "'";
    default: snprintf( buf, sizeof( buf ), "VK%d", vk ); return buf;
    }
}

struct BindEntry {
    int      id           = 0;
    int      vk           = 0;
    bool     keyPrev      = false;
    BindMode mode         = BindMode::Toggle;
    bool     showInBinds  = true;
    float    value        = 0.f;
    bool     runtimeState = false;
};

struct FeatureInfo {
    std::string            name;
    bool*                  boolPtr  = nullptr;
    float*                 floatPtr = nullptr;
    float                  minVal   = 0.f;
    float                  maxVal   = 1.f;
    bool                   hasValue = false;
    std::vector<BindEntry> binds;
    int                    nextId   = 0;
};

class BindSystem {
public:
    static BindSystem& get() {
        static BindSystem inst;
        return inst;
    }

    void registerBool(const std::string& name, bool* ptr) {
        if (m_features.count(name)) { m_features[name].boolPtr = ptr; return; }
        FeatureInfo fi;
        fi.name = name; fi.boolPtr = ptr; fi.hasValue = false;
        m_features[name] = std::move(fi);
        applyPending(name);
    }

    void registerFloat(const std::string& name, float* ptr, float mn, float mx) {
        if (m_features.count(name)) {
            auto& fi = m_features[name];
            fi.floatPtr = ptr; fi.minVal = mn; fi.maxVal = mx; return;
        }
        FeatureInfo fi;
        fi.name = name; fi.floatPtr = ptr; fi.minVal = mn; fi.maxVal = mx; fi.hasValue = true;
        m_features[name] = std::move(fi);
        applyPending(name);
    }

    // Toggle that also carries an associated value slider (e.g. Speedhack + Speed).
    // Binding it shows "New Value" and applies both the bool and the value.
    void registerValued(const std::string& name, bool* bptr, float* fptr, float mn, float mx) {
        if (m_features.count(name)) {
            auto& fi = m_features[name];
            fi.boolPtr = bptr; fi.floatPtr = fptr;
            fi.minVal = mn; fi.maxVal = mx; fi.hasValue = true;
            return;
        }
        FeatureInfo fi;
        fi.name = name; fi.boolPtr = bptr; fi.floatPtr = fptr;
        fi.minVal = mn; fi.maxVal = mx; fi.hasValue = true;
        m_features[name] = std::move(fi);
        applyPending(name);
    }

    void openPopup(const std::string& name) {
        m_bindOpen       = true;
        m_bindFeature    = name;
        m_bindAnchor     = GetMousePos();
        m_bindReposition = true;
        m_capturingKey   = false;
        if (m_features.count(name)) {
            auto& fi = m_features[name];
            if (fi.binds.empty()) addBind(fi);
            m_selectedBind[name] = ImClamp(m_selectedBind[name], 0, (int)fi.binds.size() - 1);
        }
    }

    bool rmbClicked() const { return m_rmbClicked; }

    void process() {
#ifdef _WIN32
        bool rmbDown = ( GetAsyncKeyState( 0x02 ) & 0x8000 ) != 0;
#else
        bool rmbDown = false;
#endif
        m_rmbClicked = rmbDown && !m_rmbPrevDown;
        m_rmbPrevDown = rmbDown;

        for (auto& [name, fi] : m_features) {
            for (auto& entry : fi.binds) {
                switch (entry.mode) {
                case BindMode::AlwaysOn:
                    entry.runtimeState = true;
                    applyEntry(fi, entry, true);
                    break;
                case BindMode::Toggle: {
                    if (entry.vk == 0) { entry.keyPrev = false; break; }
                    bool down = nh_vk_down(entry.vk);
                    if (down && !entry.keyPrev && !m_capturingKey) {
                        entry.runtimeState = !entry.runtimeState;
                        applyEntry(fi, entry, entry.runtimeState);
                    }
                    entry.keyPrev = down;
                    break;
                }
                case BindMode::Hold:
                    applyEntry(fi, entry, entry.vk != 0 && !m_capturingKey && nh_vk_down(entry.vk));
                    break;
                }
            }
        }
    }

    void drawBindPopup() {
        if (!m_bindOpen) return;
        if (gui.m_fade < 0.004f) { m_bindOpen = false; m_capturingKey = false; return; }
        auto* fi = getFeature(m_bindFeature);
        if (!fi) { m_bindOpen = false; return; }

        if (m_bindReposition) {
            SetNextWindowPos(m_bindAnchor, ImGuiCond_Always);
            m_bindReposition = false;
        }
        SetNextWindowSize(ImVec2(240.f, 0.f), ImGuiCond_Always);

        PushStyleVar(ImGuiStyleVar_Alpha, gui.m_fade);
        PushStyleVar(ImGuiStyleVar_WindowRounding, 6.f);
        PushStyleVar(ImGuiStyleVar_WindowPadding,  ImVec2(10.f, 10.f));
        PushStyleColor(ImGuiCol_WindowBg,      (ImU32)ImColor(0.018f, 0.030f, 0.058f, 0.99f));
        PushStyleColor(ImGuiCol_TitleBg,       (ImU32)ImColor(0.018f, 0.030f, 0.058f, 0.99f));
        PushStyleColor(ImGuiCol_TitleBgActive, (ImU32)ImColor(0.018f, 0.030f, 0.058f, 0.99f));
        PushStyleColor(ImGuiCol_Border,        (ImU32)ImColor(1.f, 1.f, 1.f, 0.08f));

        bool open = m_bindOpen;
        if (Begin("Bind##nhbw", &open,
                  ImGuiWindowFlags_NoCollapse |
                  ImGuiWindowFlags_AlwaysAutoResize |
                  ImGuiWindowFlags_NoScrollbar |
                  ImGuiWindowFlags_NoSavedSettings)) {
            m_bindOpen = open;

            if (fi->binds.empty()) addBind(*fi);
            int& sel = m_selectedBind[m_bindFeature];
            sel = ImClamp(sel, 0, (int)fi->binds.size() - 1);
            BindEntry& cur = fi->binds[sel];

            char drop_buf[64];
            snprintf(drop_buf, sizeof(drop_buf), "Bind \"%s\"", nh_vk_name(cur.vk));

            SetNextItemWidth(-1.f);
            PushStyleColor(ImGuiCol_FrameBg,        (ImU32)gui.frame_inactive.to_im_color());
            PushStyleColor(ImGuiCol_FrameBgHovered, (ImU32)gui.frame_active.to_im_color());
            PushStyleColor(ImGuiCol_PopupBg,        (ImU32)ImColor(0.018f, 0.030f, 0.058f, 0.99f));
            PushStyleColor(ImGuiCol_Header,         (ImU32)gui.accent_color.to_im_color(0.30f));
            PushStyleColor(ImGuiCol_HeaderHovered,  (ImU32)gui.button_active.to_im_color());
            if (BeginCombo("##bsel", drop_buf, ImGuiComboFlags_HeightSmall)) {
                for (int i = 0; i < (int)fi->binds.size(); ++i) {
                    auto& b = fi->binds[i];
                    char opt[64];
                    snprintf(opt, sizeof(opt), "Bind \"%s\"##bopt%d", nh_vk_name(b.vk), b.id);
                    if (Selectable(opt, sel == i)) sel = i;
                }
                Separator();
                if (Selectable("+ Add Bind")) {
                    addBind(*fi);
                    sel = (int)fi->binds.size() - 1;
                }
                EndCombo();
            }
            PopStyleColor(5);

            Spacing(); Separator(); Spacing();

            if (cur.mode != BindMode::AlwaysOn) {
            Text("Key"); SameLine(90.f);
            const bool capturing = m_capturingKey
                                && m_captureFeature == m_bindFeature
                                && m_captureBindId  == cur.id;
            if (capturing) {
                PushStyleColor(ImGuiCol_Button,        (ImU32)gui.accent_color.to_im_color(0.40f));
                PushStyleColor(ImGuiCol_ButtonHovered, (ImU32)gui.accent_color.to_im_color(0.55f));
                Button("Press any key...##kbind", ImVec2(-1.f, 0.f));
                PopStyleColor(2);
                int captured = nh_poll_pressed_vk();
                if (captured != 0) { cur.vk = captured; m_capturingKey = false; }
                if (nh_vk_down(0x1B)) m_capturingKey = false;
            } else {
                char key_buf[64];
                snprintf(key_buf, sizeof(key_buf), "%s##kbtn", nh_vk_name(cur.vk));
                if (gui.button(key_buf, ImVec2(-1.f, 0.f))) {
                    m_capturingKey   = true;
                    m_captureFeature = m_bindFeature;
                    m_captureBindId  = cur.id;
                }
            }
            }

            Spacing();
            Text("Mode"); SameLine(90.f);
            PushStyleColor(ImGuiCol_FrameBg,        (ImU32)gui.frame_inactive.to_im_color());
            PushStyleColor(ImGuiCol_FrameBgHovered, (ImU32)gui.frame_active.to_im_color());
            PushStyleColor(ImGuiCol_PopupBg,        (ImU32)ImColor(0.018f, 0.030f, 0.058f, 0.99f));
            PushStyleColor(ImGuiCol_Header,         (ImU32)gui.accent_color.to_im_color(0.30f));
            PushStyleColor(ImGuiCol_HeaderHovered,  (ImU32)gui.button_active.to_im_color());
            static const char* mode_names[] = { "Always On", "Toggle", "Hold" };
            int mode_i = (int)cur.mode;
            SetNextItemWidth(-1.f);
            if (Combo("##bmode", &mode_i, mode_names, 3)) cur.mode = (BindMode)mode_i;
            PopStyleColor(5);

            if (fi->hasValue) {
                Spacing();
                gui.slider_float("New Value", &cur.value, fi->minVal, fi->maxVal);
            }

            Spacing();
            gui.toggle("Show In Binds", &cur.showInBinds);

            {
                Spacing(); Separator(); Spacing();
                if (gui.button("Delete Bind", ImVec2(-1.f, 0.f))) {
                    fi->binds.erase(fi->binds.begin() + sel);
                    if (fi->binds.empty()) {
                        open = false; m_bindOpen = false;
                    } else {
                        sel = ImClamp(sel, 0, (int)fi->binds.size() - 1);
                    }
                }
            }

            Spacing(); Separator(); Spacing();
            if (gui.button("View all binds", ImVec2(-1.f, 0.f)))
                m_hotkeysOpen = true;
        }
        m_bindOpen = open;
        End();

        PopStyleColor(4);
        PopStyleVar(3);
    }

    void drawHotkeysList() {
        if (!m_hotkeysOpen) return;
        if (gui.m_fade < 0.004f) { m_hotkeysOpen = false; return; }

        SetNextWindowSize(ImVec2(560.f, 360.f), ImGuiCond_FirstUseEver);
        SetNextWindowPos(ImVec2(200.f, 90.f),   ImGuiCond_FirstUseEver);

        PushStyleVar(ImGuiStyleVar_Alpha,          gui.m_fade);
        PushStyleVar(ImGuiStyleVar_WindowRounding, 6.f);
        PushStyleVar(ImGuiStyleVar_WindowPadding,  ImVec2(0.f, 0.f));
        PushStyleColor(ImGuiCol_WindowBg, (ImU32)ImColor(0.012f, 0.020f, 0.045f, 1.f));
        PushStyleColor(ImGuiCol_Border,   (ImU32)gui.border.to_im_color());

        if (Begin("##nhhotkeys", nullptr,
                  ImGuiWindowFlags_NoDecoration |
                  ImGuiWindowFlags_NoSavedSettings)) {

            ImGuiWindow* window = GetCurrentWindow();
            ImDrawList*  draw   = window->DrawList;
            const ImVec2 pos    = window->Pos;
            const ImVec2 size   = window->Size;
            const float  hdr_h  = 44.f;

            draw->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + hdr_h),
                                ImColor(0.022f, 0.038f, 0.072f, gui.m_fade),
                                6.f, ImDrawFlags_RoundCornersTop);
            draw->AddLine(ImVec2(pos.x + 8.f, pos.y + hdr_h),
                          ImVec2(pos.x + size.x - 8.f, pos.y + hdr_h),
                          gui.border.to_im_color());

            draw->AddRectFilled(pos + ImVec2(14.f, 15.f), pos + ImVec2(17.f, 29.f),
                                gui.accent_color.to_im_color(), 1.f);
            draw->AddText(pos + ImVec2(27.f, 14.f), gui.accent_color.to_im_color(), "Hotkeys list");
            draw->AddText(pos + ImVec2(26.f, 14.f), GetColorU32(ImGuiCol_Text), "Hotkeys list");

            const float  cb = 22.f;
            const ImVec2 cmin(pos.x + size.x - cb - 12.f, pos.y + (hdr_h - cb) * 0.5f);
            SetCursorScreenPos(cmin);
            if (InvisibleButton("##hkclose", ImVec2(cb, cb))) m_hotkeysOpen = false;
            const bool ch = IsItemHovered();
            if (ch) draw->AddRectFilled(cmin, cmin + ImVec2(cb, cb),
                                        gui.accent_color.to_im_color(0.18f), 4.f);
            const ImVec2 cc = cmin + ImVec2(cb * 0.5f, cb * 0.5f);
            const ImU32  xcol = ch ? (ImU32)gui.accent_color.to_im_color()
                                   : (ImU32)gui.text_disabled.to_im_color();
            draw->AddLine(cc + ImVec2(-4.f, -4.f), cc + ImVec2(4.f, 4.f), xcol, 1.6f);
            draw->AddLine(cc + ImVec2(-4.f,  4.f), cc + ImVec2(4.f, -4.f), xcol, 1.6f);

            SetCursorScreenPos(ImVec2(pos.x + 14.f, pos.y + hdr_h + 12.f));
            BeginChild("##hkbody", ImVec2(size.x - 28.f, size.y - hdr_h - 24.f));

            PushStyleColor(ImGuiCol_TableHeaderBg,    (ImU32)gui.frame_inactive.to_im_color());
            PushStyleColor(ImGuiCol_TableBorderLight, (ImU32)gui.border.to_im_color(2.f));
            PushStyleColor(ImGuiCol_TableRowBg,       (ImU32)ImColor(0.f, 0.f, 0.f, 0.f));
            PushStyleColor(ImGuiCol_TableRowBgAlt,    (ImU32)ImColor(1.f, 1.f, 1.f, 0.02f));
            PushStyleVar(ImGuiStyleVar_CellPadding,   ImVec2(8.f, 6.f));

            if (BeginTable("##hktbl", 6,
                    ImGuiTableFlags_RowBg |
                    ImGuiTableFlags_BordersInnerH |
                    ImGuiTableFlags_SizingStretchProp |
                    ImGuiTableFlags_PadOuterX |
                    ImGuiTableFlags_ScrollY,
                    GetContentRegionAvail())) {

                TableSetupColumn("Feature",  ImGuiTableColumnFlags_WidthStretch, 2.2f);
                TableSetupColumn("Key",      ImGuiTableColumnFlags_WidthStretch, 1.1f);
                TableSetupColumn("Mode",     ImGuiTableColumnFlags_WidthStretch, 1.2f);
                TableSetupColumn("In Binds", ImGuiTableColumnFlags_WidthStretch, 1.0f);
                TableSetupColumn("Value",    ImGuiTableColumnFlags_WidthStretch, 1.0f);
                TableSetupColumn("##del",    ImGuiTableColumnFlags_WidthFixed,  26.f);
                TableSetupScrollFreeze(0, 1);
                TableHeadersRow();

                ImDrawList* rd = GetWindowDrawList();

                struct Row { std::string feat; int bindIdx; int bindId; };
                std::vector<Row> rows;
                for (auto& [name, fi] : m_features)
                    for (int i = 0; i < (int)fi.binds.size(); ++i) {
                        auto& b = fi.binds[i];
                        if (b.vk == 0 && b.mode != BindMode::AlwaysOn) continue;
                        rows.push_back({name, i, b.id});
                    }
                std::sort(rows.begin(), rows.end(),
                          [](const Row& a, const Row& b){ return a.bindId > b.bindId; });

                static const char* modes[] = { "Always On", "Toggle", "Hold" };
                std::string toDelName;
                int         toDelIdx = -1;

                if (rows.empty()) {
                    TableNextRow();
                    TableSetColumnIndex(0);
                    PushStyleColor(ImGuiCol_Text, (ImU32)gui.text_disabled.to_im_color());
                    TextUnformatted("No binds yet - right-click a feature to add one");
                    PopStyleColor();
                }

                for (int ri = 0; ri < (int)rows.size(); ++ri) {
                    auto* fi = getFeature(rows[ri].feat);
                    if (!fi || rows[ri].bindIdx >= (int)fi->binds.size()) continue;
                    auto& entry = fi->binds[rows[ri].bindIdx];

                    TableNextRow();
                    TableSetColumnIndex(0); TextUnformatted(fi->name.c_str());

                    TableSetColumnIndex(1);
                    if (entry.vk != 0) {
                        const char*  kn  = nh_vk_name(entry.vk);
                        const ImVec2 ksz = CalcTextSize(kn);
                        const ImVec2 kp  = GetCursorScreenPos();
                        rd->AddRectFilled(ImVec2(kp.x - 4.f, kp.y - 1.f),
                                          ImVec2(kp.x + ksz.x + 4.f, kp.y + ksz.y + 1.f),
                                          gui.frame_active.to_im_color(), 3.f);
                        TextUnformatted(kn);
                    } else {
                        PushStyleColor(ImGuiCol_Text, (ImU32)gui.text_disabled.to_im_color());
                        TextUnformatted("-");
                        PopStyleColor();
                    }

                    TableSetColumnIndex(2);
                    PushStyleColor(ImGuiCol_Text, (ImU32)gui.accent_color.to_im_color());
                    TextUnformatted(modes[(int)entry.mode]);
                    PopStyleColor();

                    TableSetColumnIndex(3);
                    Text("%s", entry.showInBinds ? "Show" : "Hidden");

                    TableSetColumnIndex(4);
                    if (fi->hasValue) Text("%.2f", entry.value);
                    else              Text("%s", entry.runtimeState ? "on" : "off");

                    TableSetColumnIndex(5);
                    char del_id[32];
                    snprintf(del_id, sizeof(del_id), "##hkd%d", ri);
                    const ImVec2 dp = GetCursorScreenPos();
                    if (InvisibleButton(del_id, ImVec2(18.f, 18.f))) {
                        toDelName = rows[ri].feat; toDelIdx = rows[ri].bindIdx;
                    }
                    const bool   dh  = IsItemHovered();
                    const ImVec2 dc  = ImVec2(dp.x + 9.f, dp.y + 9.f);
                    if (dh) rd->AddRectFilled(dp, ImVec2(dp.x + 18.f, dp.y + 18.f),
                                              gui.accent_color.to_im_color(0.20f), 3.f);
                    const ImU32 dcol = dh ? (ImU32)gui.accent_color.to_im_color()
                                          : (ImU32)gui.text_disabled.to_im_color();
                    rd->AddLine(ImVec2(dc.x - 3.5f, dc.y - 3.5f), ImVec2(dc.x + 3.5f, dc.y + 3.5f), dcol, 1.5f);
                    rd->AddLine(ImVec2(dc.x - 3.5f, dc.y + 3.5f), ImVec2(dc.x + 3.5f, dc.y - 3.5f), dcol, 1.5f);
                }

                if (toDelIdx >= 0) {
                    auto* fi = getFeature(toDelName);
                    if (fi) fi->binds.erase(fi->binds.begin() + toDelIdx);
                }
                EndTable();
            }

            PopStyleVar(1);
            PopStyleColor(4);
            EndChild();
        }
        End();

        PopStyleColor(2);
        PopStyleVar(3);
    }

    void drawBindsOverlay() {
        if (!Vars::keybindsList) return;

        ImGuiIO&    io   = GetIO();
        ImDrawList* draw = GetForegroundDrawList();
        const bool  menuShown = gui.m_fade > 0.5f;

        struct Row { std::string label; std::string key; float a; float target; };
        static std::vector<Row> rows;

        // gather active binds + measure ALL configured binds for a fixed width
        struct Active { std::string label; std::string key; };
        std::vector<Active> active;
        float maxLabelW = 0.f, maxKeyW = 0.f;

        for (auto& [name, fi] : m_features) {
            for (auto& entry : fi.binds) {
                if (!entry.showInBinds) continue;
                if (entry.vk != 0 || entry.mode == BindMode::AlwaysOn) {
                    std::string k = (entry.mode == BindMode::AlwaysOn)
                        ? std::string("Always") : std::string(nh_vk_name(entry.vk));
                    maxLabelW = ImMax(maxLabelW, CalcTextSize(fi.name.c_str()).x);
                    maxKeyW   = ImMax(maxKeyW,   CalcTextSize(k.c_str()).x);
                }
                bool visible = false;
                switch (entry.mode) {
                case BindMode::AlwaysOn: visible = true; break;
                case BindMode::Hold:     visible = entry.vk != 0 && nh_vk_down(entry.vk); break;
                case BindMode::Toggle:   visible = entry.vk != 0 && entry.runtimeState;   break;
                }
                if (!visible) continue;
                std::string k = (entry.mode == BindMode::AlwaysOn)
                    ? std::string("Always") : std::string(nh_vk_name(entry.vk));
                active.push_back({fi.name, k});
            }
        }

        // per-row appear/disappear animation
        for (auto& r : rows) r.target = 0.f;
        for (auto& ac : active) {
            bool found = false;
            for (auto& r : rows)
                if (r.label == ac.label && r.key == ac.key) { r.target = 1.f; found = true; break; }
            if (!found) rows.push_back({ac.label, ac.key, 0.f, 1.f});
        }
        const float dt   = io.DeltaTime > 0.f ? io.DeltaTime : 1.f / 60.f;
        const float lerp = ImMin(1.f, dt * 12.f);
        for (auto& r : rows) r.a += (r.target - r.a) * lerp;
        rows.erase(std::remove_if(rows.begin(), rows.end(),
            [](const Row& r){ return r.target == 0.f && r.a < 0.01f; }), rows.end());

        // choose render set: real rows, or a faded preview while the menu is open
        std::vector<Row>  previewRows;
        std::vector<Row>* useRows;
        float globalMul = 1.f;
        if (!rows.empty()) {
            useRows = &rows;
        } else if (menuShown) {
            previewRows.push_back({ std::string("Noclip"), std::string("Ctrl"), 1.f, 1.f });
            useRows   = &previewRows;
            globalMul = 0.5f;
        } else {
            return;
        }

        for (auto& r : *useRows) {
            maxLabelW = ImMax(maxLabelW, CalcTextSize(r.label.c_str()).x);
            maxKeyW   = ImMax(maxKeyW,   CalcTextSize(r.key.c_str()).x);
        }

        const int   style     = Vars::keybindsStyle;
        const char* title     = "Keybinds";
        const float text_h    = CalcTextSize("A").y;
        const float pad_x     = 12.f;
        const float pad_y     = 7.f;
        const float cap_pad   = 7.f;
        const float col_gap   = 26.f;
        const float row_h     = text_h + 8.f;
        const float row_gap   = 4.f;
        const float top_pad   = (style == 2) ? 6.f : 0.f;
        const float key_col_w = maxKeyW + cap_pad * 2.f;

        float content_w = ImMax(CalcTextSize(title).x, maxLabelW + col_gap + key_col_w);
        content_w = ImMax(content_w, 150.f);

        float rows_h  = 0.f;
        float globalA = 0.f;
        for (auto& r : *useRows) {
            rows_h  += (row_h + row_gap) * r.a;
            globalA  = ImMax(globalA, r.a);
        }
        globalA *= globalMul;
        if (globalA < 0.004f) return;

        const float  header_h = text_h + 6.f;
        const ImVec2 box_sz(content_w + pad_x * 2.f,
                            top_pad + pad_y + header_h + rows_h + pad_y);

        float bx = Vars::keybindsX * io.DisplaySize.x;
        float by = Vars::keybindsY * io.DisplaySize.y;
        bx = ImClamp(bx, 0.f, ImMax(0.f, io.DisplaySize.x - box_sz.x));
        by = ImClamp(by, 0.f, ImMax(0.f, io.DisplaySize.y - box_sz.y));

        ImVec2 box_pos(bx, by);
        ImVec2 box_end(bx + box_sz.x, by + box_sz.y);

        // dragging (only while the menu is open). Use a transparent ImGui window
        // + InvisibleButton so it relies on ImGui's own input path (the same one
        // the menu widgets use) instead of io.WantCaptureMouse, which the cocos
        // integration forces to true while the menu is open.
        if (menuShown) {
            PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));
            PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
            SetNextWindowPos(box_pos);
            SetNextWindowSize(box_sz);
            Begin("##kbgrab", nullptr,
                  ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                  ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBackground |
                  ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav);
            InvisibleButton("##kbgrabbtn", box_sz);
            if (IsItemActive()) {
                bx = ImClamp(bx + io.MouseDelta.x, 0.f, ImMax(0.f, io.DisplaySize.x - box_sz.x));
                by = ImClamp(by + io.MouseDelta.y, 0.f, ImMax(0.f, io.DisplaySize.y - box_sz.y));
                if (io.DisplaySize.x > 0.f) Vars::keybindsX = bx / io.DisplaySize.x;
                if (io.DisplaySize.y > 0.f) Vars::keybindsY = by / io.DisplaySize.y;
                box_pos = ImVec2(bx, by);
                box_end = ImVec2(bx + box_sz.x, by + box_sz.y);
            }
            End();
            PopStyleVar(2);
        }

        const ImVec4 c1(Vars::wmColorOne[0], Vars::wmColorOne[1], Vars::wmColorOne[2], Vars::wmColorOne[3]);
        const ImVec4 c2(Vars::wmColorTwo[0], Vars::wmColorTwo[1], Vars::wmColorTwo[2], Vars::wmColorTwo[3]);
        const float  gphase = (float)GetTime();
        const bool   line_gradient = (Vars::keybindsLine == 1);

        auto mulA = [](ImU32 c, float a) -> ImU32 {
            int al = (int)((float)((c >> IM_COL32_A_SHIFT) & 0xFF) * a);
            if (al < 0) al = 0; if (al > 255) al = 255;
            return (c & ~IM_COL32_A_MASK) | ((ImU32)al << IM_COL32_A_SHIFT);
        };
        const ImU32 accent_base = Vars::kbGlobal ? (ImU32)gui.accent_color.to_im_color() : (ImU32)ImColor(c1);
        const ImU32 accent_box  = mulA(accent_base, globalA);

        auto faded = [&](float alpha) -> ImU32 {
            ImU32 b = Vars::kbGlobal ? (ImU32)gui.accent_color.to_im_color(alpha)
                                     : (ImU32)ImColor(c1.x, c1.y, c1.z, alpha);
            return mulA(b, globalA);
        };
        auto draw_line = [&](float x0, float x1, float y0, float th) {
            if (line_gradient) {
                const float kk = 0.5f + 0.5f * sinf(gphase * 2.f);
                const ImU32 cl = mulA((ImU32)ImColor(ImLerp(c1, c2, kk)), globalA);
                const ImU32 cr = mulA((ImU32)ImColor(ImLerp(c2, c1, kk)), globalA);
                draw->AddRectFilledMultiColor(ImVec2(x0, y0), ImVec2(x1, y0 + th), cl, cr, cr, cl);
            } else {
                draw->AddRectFilled(ImVec2(x0, y0), ImVec2(x1, y0 + th), accent_box);
            }
        };

        const float rounding = (style == 0) ? 3.f : 0.f;
        float content_top = box_pos.y + top_pad + pad_y;

        if (style == 2) {
            // Skeet: dark box + double border + rainbow gradient top lines
            if (Vars::kbBackground) {
                draw->AddRectFilled(box_pos, box_end, mulA((ImU32)ImColor(20, 20, 20, 235), globalA));
                draw->AddRect(box_pos, box_end, mulA((ImU32)ImColor(18, 18, 18, 255), globalA));
                draw->AddRect(ImVec2(box_pos.x + 1.f, box_pos.y + 1.f),
                              ImVec2(box_end.x - 1.f, box_end.y - 1.f),
                              mulA((ImU32)ImColor(62, 62, 62, 255), globalA));
            }
            const float lx0 = box_pos.x + 3.f, lx1 = box_end.x - 3.f;
            const float ly  = box_pos.y + 3.f;
            const float mid = (lx0 + lx1) * 0.5f;
            for (int r = 0; r < 2; ++r) {
                const float yy = ly + (float)r;
                const float f  = (r == 0) ? 1.f : 0.5f;
                const ImU32 blue   = mulA((ImU32)ImColor((int)(59 * f),  (int)(175 * f), (int)(222 * f), 255), globalA);
                const ImU32 purple = mulA((ImU32)ImColor((int)(202 * f), (int)(70 * f),  (int)(205 * f), 255), globalA);
                const ImU32 green  = mulA((ImU32)ImColor((int)(201 * f), (int)(227 * f), (int)(58 * f),  255), globalA);
                draw->AddRectFilledMultiColor(ImVec2(lx0, yy), ImVec2(mid, yy + 1.f), blue, purple, purple, blue);
                draw->AddRectFilledMultiColor(ImVec2(mid, yy), ImVec2(lx1, yy + 1.f), purple, green, green, purple);
            }
            content_top = ly + 3.f + pad_y;
        }
        else {
            if (Vars::kbBackground)
                draw->AddRectFilled(box_pos, box_end, mulA((ImU32)ImColor(0.f, 0.f, 0.f, 0.55f), globalA), rounding);

            if (style == 1) {
                // Onetap v3: top line + glow
                if (Vars::kbGlow)
                    for (int i = 4; i >= 1; --i)
                        draw->AddRectFilled(ImVec2(box_pos.x, box_pos.y - (float)i),
                                            ImVec2(box_end.x, box_pos.y + 1.f + (float)i),
                                            faded(0.05f));
                draw_line(box_pos.x, box_end.x, box_pos.y, 1.5f);
            } else {
                // Version 1.5: full rounded accent border + glow
                if (Vars::kbGlow)
                    for (int i = 5; i >= 1; --i) {
                        const float e = (float)i * 2.f;
                        draw->AddRect(ImVec2(box_pos.x - e, box_pos.y - e),
                                      ImVec2(box_end.x + e, box_end.y + e),
                                      faded(0.045f / (float)i), rounding + e, 0, 2.f);
                    }
                draw->AddRect(box_pos, box_end, accent_box, rounding, 0, 1.5f);
            }
        }

        const ImU32 col_text = (ImU32)gui.text.to_im_color();

        const float title_w = CalcTextSize(title).x;
        draw->AddText(ImVec2(box_pos.x + (box_sz.x - title_w) * 0.5f, content_top),
                      mulA(accent_base, globalA), title);

        float ry = content_top + header_h;
        const float key_x_right = box_end.x - pad_x;
        for (auto& r : *useRows) {
            const float a = r.a;
            if (a < 0.004f) continue;
            const float slide = (1.f - a) * 14.f;
            const float ty    = ry + (row_h - text_h) * 0.5f;
            const float ca    = globalMul * a;

            draw->AddText(ImVec2(box_pos.x + pad_x + slide, ty), mulA(col_text, ca), r.label.c_str());

            const ImVec2 cap_max(key_x_right - slide, ry + row_h - 2.f);
            const ImVec2 cap_min(cap_max.x - key_col_w, ry + 2.f);
            draw->AddRectFilled(cap_min, cap_max, mulA((ImU32)gui.frame_active.to_im_color(), ca), 3.f);
            draw->AddRect(cap_min, cap_max, mulA((ImU32)gui.border.to_im_color(2.5f), ca), 3.f);
            const float kw = CalcTextSize(r.key.c_str()).x;
            draw->AddText(ImVec2(cap_min.x + (key_col_w - kw) * 0.5f, ty), mulA(col_text, ca), r.key.c_str());

            ry += (row_h + row_gap) * a;
        }
    }

    void save(geode::Mod* mod) {
        std::vector<matjson::Value> featArr;
        for (auto& [name, fi] : m_features) {
            if (fi.binds.empty()) continue;
            std::vector<matjson::Value> bindArr;
            for (auto& b : fi.binds) {
                if (b.vk == 0 && b.mode != BindMode::AlwaysOn) continue;
                matjson::Value bv = matjson::Value::object();
                bv["id"] = (int64_t)b.id;
                bv["k"]  = (int64_t)b.vk;
                bv["m"]  = (int64_t)(int)b.mode;
                bv["s"]  = b.showInBinds;
                bv["v"]  = (double)b.value;
                bindArr.push_back(std::move(bv));
            }
            if (bindArr.empty()) continue;
            matjson::Value fv = matjson::Value::object();
            fv["n"]     = name;
            fv["b"]     = matjson::Value(std::move(bindArr));
            fv["nxtid"] = (int64_t)fi.nextId;
            featArr.push_back(std::move(fv));
        }
        mod->setSavedValue("binds_v1", matjson::Value(std::move(featArr)));
    }

    void load(geode::Mod* mod) {
        auto root = mod->getSavedValue<matjson::Value>("binds_v1", matjson::Value::array());
        if (!root.isArray()) return;
        auto rootArr = root.asArray();
        if (!rootArr.isOk()) return;
        for (auto& fv : rootArr.unwrap()) {
            if (!fv.isObject()) continue;
            if (!fv.contains("n") || !fv["n"].isString()) continue;
            std::string name = fv["n"].asString().unwrapOr(std::string{});
            if (name.empty()) continue;
            if (!fv.contains("b") || !fv["b"].isArray()) continue;

            int nextId = 0;
            if (fv.contains("nxtid")) nextId = (int)fv["nxtid"].asInt().unwrapOr(0);

            std::vector<BindEntry> entries;
            auto bArr = fv["b"].asArray();
            if (!bArr.isOk()) continue;
            for (auto& bv : bArr.unwrap()) {
                if (!bv.isObject()) continue;
                BindEntry e;
                if (bv.contains("id")) e.id          = (int)bv["id"].asInt().unwrapOr(nextId);
                if (bv.contains("k"))  e.vk          = (int)bv["k"].asInt().unwrapOr(0);
                if (bv.contains("m"))  e.mode        = (BindMode)(int)bv["m"].asInt().unwrapOr(1);
                if (bv.contains("s"))  e.showInBinds = bv["s"].asBool().unwrapOr(true);
                if (bv.contains("v"))  e.value       = (float)bv["v"].asDouble().unwrapOr(0.0);
                nextId = ImMax(nextId, e.id + 1);
                entries.push_back(e);
            }
            if (m_features.count(name)) {
                m_features[name].binds  = std::move(entries);
                m_features[name].nextId = nextId;
            } else {
                m_pending[name]       = std::move(entries);
                m_pendingNextId[name] = nextId;
            }
        }
    }

    void registerAll() {
        registerBool("Noclip", &Vars::noclip);
        registerBool("Noclip Tint", &Vars::noclipTint);
        registerBool("Noclip Hitsound", &Vars::noclipHitsound);
        registerBool("No Death Effect", &Vars::noDeathEffect);
        registerBool("No Respawn Flash", &Vars::noRespawnFlash);
        registerBool("No Pause Button", &Vars::noPauseButton);
        registerValued("Speedhack", &Vars::speedhack, &Vars::speedhackValue, 0.1f, 5.0f);
        registerBool("Speedhack Audio", &Vars::speedhackAudio);
        registerValued("FPS Bypass", &Vars::fpsUnlock, &Vars::fpsValue, 30.0f, 1000.0f);
        registerValued("TPS Bypass", &Vars::tpsBypass, &Vars::tpsValue, 1.0f, 10000.0f);
        // registerBool("Compact Lists", &Vars::compactLists);
        registerValued("Player 1", &Vars::autoclicker, &Vars::autoclickerCps, 1.0f, 30.0f);
        registerValued("Player 2", &Vars::autoclickerP2, &Vars::autoclickerP2Cps, 1.0f, 30.0f);
        registerBool("Hide Attempts", &Vars::hideAttempts);
        registerBool("No Glow", &Vars::noGlow);
        // registerBool("No Camera Shake", &Vars::noCameraShake);
        // registerBool("No End Shake", &Vars::noEndShake);
        registerBool("No Dash Fire", &Vars::noDashFire);
        registerBool("No Spider Dash", &Vars::noSpiderDash);
        registerBool("No Particles", &Vars::noParticles);
        registerBool("No Trail", &Vars::noTrail);
        registerBool("No Wave Pulse", &Vars::noWavePulse);
        registerBool("No Wave Trail", &Vars::noWaveTrail);
        registerBool("Solid Wave Trail", &Vars::solidWaveTrail);
        registerValued("Wave Trail Size", &Vars::waveTrailSize, &Vars::waveTrailSizeValue, 0.1f, 5.0f);
        registerBool("Accurate Percentage", &Vars::accuratePercent);
        registerBool("Keybinds", &Vars::keybindsList);
        registerBool("Show Hitboxes", &Vars::showHitboxes);
        registerBool("Show On Death", &Vars::showHitboxesOnDeath);
        registerBool("Trajectory Prediction", &Vars::showTrajectory);
        registerBool("No Shader", &Vars::noShader);
        // registerBool("No Portal Lightning", &Vars::noPortalLightning);
        registerBool("Hide Complete VFX", &Vars::hideLevelCompleteVfx);
        registerBool("No Music Fade Out", &Vars::noMusicFadeOut);
        registerBool("Auto Practice Mode", &Vars::autoPracticeMode);
        registerBool("Auto Pickup Coins", &Vars::autoPickupCoins);
        registerBool("Pause On Complete", &Vars::pauseDuringComplete);
        registerBool("Auto Song Download", &Vars::autoSongDownload);
        registerBool("Layout Mode", &Vars::layoutMode);
        registerBool("Unlock Main Levels", &Vars::unlockMainLevels);
        registerBool("Unlock Shops", &Vars::unlockShops);
        registerBool("Unlock Vaults", &Vars::unlockVaults);
        registerBool("Hitbox Multiplier", &Vars::hitboxMultiplier);
        registerBool("Practice Music", &Vars::practiceMusic);
        registerBool("Icon Bypass", &Vars::iconBypass);
        registerBool("No Transition", &Vars::noTransition);
        registerBool("Safe Mode", &Vars::safeMode);
        registerBool("Freeze Attempts", &Vars::safeFreezeAttempts);
        registerBool("Freeze Jumps", &Vars::safeFreezeJumps);
        registerBool("Instant Complete", &Vars::instantComplete);
        registerBool("No Mirror Portal", &Vars::noMirrorPortal);
        registerBool("Instant Restart", &Vars::instantRestart);
        registerBool("Custom Respawn", &Vars::customRespawn);
        registerBool("Jump Hack", &Vars::jumpHack);
        registerBool("All Modes Platformer", &Vars::allModesPlatformer);
        registerBool("Verify Hack", &Vars::verifyHack);
        registerBool("Copy Hack", &Vars::copyHack);
        registerBool("Hide Editor UI", &Vars::hideEditorUI);
        registerBool("Level Edit", &Vars::levelEdit);
        registerBool("No Custom Obj Limit", &Vars::noCustomObjLimit);
        registerBool("No Zoom Limit", &Vars::noZoomLimit);
        registerBool("Toolbox Button Bypass", &Vars::toolboxButtonBypass);
        registerBool("Slider Limit Bypass", &Vars::sliderLimitBypass);

        registerFloat("Speed", &Vars::speedhackValue, 0.1f, 5.0f);
        registerFloat("FPS", &Vars::fpsValue, 30.0f, 1000.0f);
        registerFloat("P1 CPS", &Vars::autoclickerCps, 1.0f, 30.0f);
        registerFloat("P2 CPS", &Vars::autoclickerP2Cps, 1.0f, 30.0f);
        registerFloat("Size", &Vars::waveTrailSizeValue, 0.1f, 5.0f);
        registerFloat("Player", &Vars::hitboxMultPlayer, 0.1f, 3.0f);
        registerFloat("Solids", &Vars::hitboxMultSolid, 0.1f, 3.0f);
        registerFloat("Hazards", &Vars::hitboxMultHazard, 0.1f, 3.0f);
    }

private:
    std::unordered_map<std::string, FeatureInfo>            m_features;
    std::unordered_map<std::string, std::vector<BindEntry>> m_pending;
    std::unordered_map<std::string, int>                    m_pendingNextId;

    bool        m_rmbPrevDown    = false;
    bool        m_rmbClicked     = false;
    bool        m_bindOpen       = false;
    std::string m_bindFeature;
    ImVec2      m_bindAnchor{};
    bool        m_bindReposition = false;
    bool        m_hotkeysOpen    = false;
    bool        m_capturingKey   = false;
    std::string m_captureFeature;
    int         m_captureBindId  = -1;
    std::unordered_map<std::string, int> m_selectedBind;

    FeatureInfo* getFeature(const std::string& name) {
        auto it = m_features.find(name);
        return it != m_features.end() ? &it->second : nullptr;
    }

    void addBind(FeatureInfo& fi) {
        BindEntry e;
        e.id = fi.nextId++;
        e.mode = BindMode::Toggle;
        e.showInBinds = true;
        if (fi.hasValue && fi.floatPtr) e.value = *fi.floatPtr;
        fi.binds.push_back(e);
    }

    void applyEntry(FeatureInfo& fi, BindEntry& entry, bool active) {
        if (fi.boolPtr) *fi.boolPtr = active;
        if (fi.hasValue && fi.floatPtr && active)
            *fi.floatPtr = ImClamp(entry.value, fi.minVal, fi.maxVal);
    }

    void applyPending(const std::string& name) {
        auto it = m_pending.find(name);
        if (it == m_pending.end()) return;
        m_features[name].binds  = std::move(it->second);
        m_features[name].nextId = m_pendingNextId.count(name) ? m_pendingNextId[name] : 0;
        m_pending.erase(it);
        m_pendingNextId.erase(name);
    }
};
