#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>

#include "../Config.hpp"

using namespace geode::prelude;

namespace nh::cbf {

namespace {
    int  g_lastApplied = -1;
    bool g_modChecked  = false;
    Mod* g_cbfMod      = nullptr;

    Mod* cbfMod() {
        if (!g_modChecked) {
            g_modChecked = true;
            g_cbfMod = Loader::get()->getLoadedMod("syzzi.click_between_frames");
        }
        return g_cbfMod;
    }
}

bool available() {
    return cbfMod() != nullptr;
}

void applyToLayer(GJBaseGameLayer* layer) {
    if (!layer) return;

    const bool on = Config::get().clickBetweenFrames;
    layer->m_clickBetweenSteps = on;
    layer->m_clickOnSteps      = on;
}

void sync(bool force) {
    const bool on = Config::get().clickBetweenFrames;
    const int  state = on ? 1 : 0;

    if (!force && state == g_lastApplied)
        return;
    g_lastApplied = state;

    if (Mod* mod = cbfMod())
        (void)mod->setSettingValue<bool>("soft-toggle", !on);

    applyToLayer(GJBaseGameLayer::get());
}

}

class $modify(NHClickBetweenFrames, GJBaseGameLayer) {
    void resetLevelVariables() {
        GJBaseGameLayer::resetLevelVariables();
        nh::cbf::applyToLayer(this);
    }

    void update(float dt) {
        GJBaseGameLayer::update(dt);
        nh::cbf::sync(false);
    }
};
