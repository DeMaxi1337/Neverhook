#include <Geode/Geode.hpp>
#include <Geode/modify/CCKeyboardDispatcher.hpp>
#include <imgui-cocos.hpp>

#include "gui/framework_gui.h"
#include "gui/vars.h"

using namespace geode::prelude;

$on_mod(Loaded) {
    ImGuiCocos::get()
        .setup([] { FrameWorkInit(); })
        .draw([]  { DrawFrameWorkGUI(); });
}

class $modify(NHKeyboard, cocos2d::CCKeyboardDispatcher) {
    bool dispatchKeyboardMSG(cocos2d::enumKeyCodes key, bool isKeyDown, bool isKeyRepeat, double timestamp) {
        if (key == cocos2d::enumKeyCodes::KEY_Insert && isKeyDown && !isKeyRepeat) {
            Vars::menuOpen = !Vars::menuOpen;
            return true;
        }
        return CCKeyboardDispatcher::dispatchKeyboardMSG(key, isKeyDown, isKeyRepeat, timestamp);
    }
};
