#include <Geode/Geode.hpp>
#include <Geode/modify/CCKeyboardDispatcher.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <Geode/modify/EditorPauseLayer.hpp>
#include <Geode/modify/UILayer.hpp>
#include <imgui-cocos.hpp>
#include <imgui_internal.h>

#include "gui/framework_gui.h"
#include "gui/vars.h"

using namespace geode::prelude;

$on_mod(Loaded) {
    ImGuiCocos::get()
        .setup([] { FrameWorkInit(); })
        .draw([]  { DrawFrameWorkGUI(); });
}

class $modify(NHKeyboard, cocos2d::CCKeyboardDispatcher) {
    static void onModify(auto& self) {
        (void)self.setHookPriority("cocos2d::CCKeyboardDispatcher::dispatchKeyboardMSG", 999999);
    }

    bool dispatchKeyboardMSG(cocos2d::enumKeyCodes key, bool isKeyDown, bool isKeyRepeat, double timestamp) {
        if ((key == cocos2d::enumKeyCodes::KEY_Insert || key == cocos2d::enumKeyCodes::KEY_Tab) && isKeyDown && !isKeyRepeat) {
            Vars::menuOpen = !Vars::menuOpen;
            ImGui::ClearActiveID();
            ImGui::SetWindowFocus(nullptr);
            return true;
        }

        if (Vars::menuOpen) {
            if (key == cocos2d::enumKeyCodes::KEY_Escape && isKeyDown && !isKeyRepeat) {
                Vars::menuOpen = false;
                ImGui::ClearActiveID();
                ImGui::SetWindowFocus(nullptr);
                return true;
            }
            return true;
        }

        if (ImGui::GetIO().WantCaptureKeyboard || ImGui::GetIO().WantTextInput) {
            return true;
        }

        return CCKeyboardDispatcher::dispatchKeyboardMSG(key, isKeyDown, isKeyRepeat, timestamp);
    }
};

class $modify(NHMenuBGL, GJBaseGameLayer) {
    static void onModify(auto& self) {
        (void)self.setHookPriority("GJBaseGameLayer::handleButton", 999999);
    }

    void handleButton(bool down, int button, bool player1) {
        if (Vars::menuOpen || ImGui::GetIO().WantCaptureMouse || ImGui::GetIO().WantCaptureKeyboard || ImGui::GetIO().WantTextInput)
            return;
        GJBaseGameLayer::handleButton(down, button, player1);
    }
};

class $modify(NHPauseLayer, PauseLayer) {
    static void onModify(auto& self) {
        (void)self.setHookPriority("PauseLayer::keyDown", 999999);
        (void)self.setHookPriority("PauseLayer::onResume", 999999);
    }

    void keyDown(cocos2d::enumKeyCodes key, double timestamp) {
        if (Vars::menuOpen || ImGui::GetIO().WantCaptureKeyboard || ImGui::GetIO().WantTextInput)
            return;
        PauseLayer::keyDown(key, timestamp);
    }

    void onResume(cocos2d::CCObject* sender) {
        if (Vars::menuOpen || ImGui::GetIO().WantCaptureKeyboard || ImGui::GetIO().WantTextInput)
            return;
        PauseLayer::onResume(sender);
    }
};

class $modify(NHEditorPauseLayer, EditorPauseLayer) {
    static void onModify(auto& self) {
        (void)self.setHookPriority("EditorPauseLayer::keyDown", 999999);
        (void)self.setHookPriority("EditorPauseLayer::onResume", 999999);
    }

    void keyDown(cocos2d::enumKeyCodes key, double timestamp) {
        if (Vars::menuOpen || ImGui::GetIO().WantCaptureKeyboard || ImGui::GetIO().WantTextInput)
            return;
        EditorPauseLayer::keyDown(key, timestamp);
    }

    void onResume(cocos2d::CCObject* sender) {
        if (Vars::menuOpen || ImGui::GetIO().WantCaptureKeyboard || ImGui::GetIO().WantTextInput)
            return;
        EditorPauseLayer::onResume(sender);
    }
};

class $modify(NHUILayerKey, UILayer) {
    static void onModify(auto& self) {
        (void)self.setHookPriority("UILayer::keyDown", 999999);
        (void)self.setHookPriority("UILayer::handleKeypress", 999999);
    }

    void keyDown(cocos2d::enumKeyCodes key, double timestamp) {
        if (Vars::menuOpen || ImGui::GetIO().WantCaptureKeyboard || ImGui::GetIO().WantTextInput)
            return;
        UILayer::keyDown(key, timestamp);
    }

    void handleKeypress(cocos2d::enumKeyCodes key, bool down, double timestamp) {
        if (Vars::menuOpen || ImGui::GetIO().WantCaptureKeyboard || ImGui::GetIO().WantTextInput)
            return;
        UILayer::handleKeypress(key, down, timestamp);
    }
};
