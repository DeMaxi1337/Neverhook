
#include "LuaEngine.hpp"

#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>

using namespace geode::prelude;

class $modify(NHLuaPlayLayer, PlayLayer) {

    bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
        if (!PlayLayer::init(level, useReplay, dontCreateObjects))
            return false;

        nh::lua::Manager::get().dispatch(nh::lua::Event::LevelStart);
        return true;
    }

    void destroyPlayer(PlayerObject* player, GameObject* object) {
        PlayLayer::destroyPlayer(player, object);
        nh::lua::Manager::get().dispatch(nh::lua::Event::Death);
    }

    void levelComplete() {
        PlayLayer::levelComplete();
        nh::lua::Manager::get().dispatch(nh::lua::Event::LevelEnd);
    }

    void resetLevel() {
        PlayLayer::resetLevel();
        nh::lua::Manager::get().dispatch(nh::lua::Event::Reset);
        nh::lua::Manager::get().dispatch(nh::lua::Event::Attempt);
    }
};

namespace nh::lua {

namespace {

template <class T>
concept HasActivatedCheckpoint = requires(T* p) { p->m_activatedCheckpoint; };

const void* currentCheckpoint(PlayLayer* pl) {
    if constexpr (HasActivatedCheckpoint<PlayLayer>) {
        return static_cast<const void*>(pl->m_activatedCheckpoint);
    }
    else {
        return nullptr;
    }
}

const void* g_lastCheckpoint = nullptr;
int         g_lastPercent    = -1;
bool        g_wasInLevel     = false;

}

void pollLevelEvents() {
    auto* pl = PlayLayer::get();

    if (!pl) {
        g_lastCheckpoint = nullptr;
        g_lastPercent    = -1;
        g_wasInLevel     = false;
        return;
    }

    if (!g_wasInLevel) {

        g_wasInLevel     = true;
        g_lastCheckpoint = currentCheckpoint(pl);
        g_lastPercent    = (int)pl->getCurrentPercent();
        return;
    }

    const int percent = (int)pl->getCurrentPercent();
    if (percent != g_lastPercent) {
        g_lastPercent = percent;
        Manager::get().dispatch(Event::Percent);
    }

    const void* cp = currentCheckpoint(pl);
    if (cp != g_lastCheckpoint) {
        g_lastCheckpoint = cp;
        if (cp) Manager::get().dispatch(Event::Checkpoint);
    }
}

}
