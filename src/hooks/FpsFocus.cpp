#include <Geode/Geode.hpp>
#include <Geode/modify/AppDelegate.hpp>
#include "../Config.hpp"

using namespace geode::prelude;

class $modify(NHFpsFocus, AppDelegate) {
    void applicationWillEnterForeground() {
        AppDelegate::applicationWillEnterForeground();
        applyFPS();
    }
};
