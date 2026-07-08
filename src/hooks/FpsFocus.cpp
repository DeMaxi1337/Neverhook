#include <Geode/Geode.hpp>
#include <Geode/modify/AppDelegate.hpp>
#include "../Config.hpp"

using namespace geode::prelude;

// When the window loses and regains focus (alt-tab / minimize), GD resets the
// animation interval back to 60 FPS. Re-apply the user's FPS setting every time
// we return to the foreground so FPS Bypass survives minimizing.
class $modify(NHFpsFocus, AppDelegate) {
    void applicationWillEnterForeground() {
        AppDelegate::applicationWillEnterForeground();
        applyFPS();
    }
};
