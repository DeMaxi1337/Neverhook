#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <Geode/modify/CCMouseDispatcher.hpp>
#include <Geode/modify/CCScheduler.hpp>
#include <imgui.h>
#include "../Config.hpp"
#include "../gui/vars.h"

#ifdef _WIN32
extern "C" __declspec(dllimport) short __stdcall GetAsyncKeyState(int vKey);
#endif

using namespace geode::prelude;

namespace {

CCPoint s_lastMouse = { 0.f, 0.f };

void resetPlayLayerZoom() {
    auto* pl = PlayLayer::get();
    if (pl) {
        pl->setScale(1.0f);
        pl->setPosition({ 0.f, 0.f });
    }
}

}

class $modify(NHPauseZoomPlayLayer, PlayLayer) {
    void resume() {
        resetPlayLayerZoom();
        PlayLayer::resume();
    }

    void resetLevel() {
        resetPlayLayerZoom();
        PlayLayer::resetLevel();
    }

    void onQuit() {
        resetPlayLayerZoom();
        PlayLayer::onQuit();
    }
};

class $modify(NHPauseZoomPauseLayer, PauseLayer) {
    void onResume(CCObject* sender) {
        resetPlayLayerZoom();
        PauseLayer::onResume(sender);
    }

    void onRestart(CCObject* sender) {
        resetPlayLayerZoom();
        PauseLayer::onRestart(sender);
    }

    void onRestartFull(CCObject* sender) {
        resetPlayLayerZoom();
        PauseLayer::onRestartFull(sender);
    }
};

class $modify(NHPauseZoomScheduler, CCScheduler) {
    virtual void update(float dt) {
        CCScheduler::update(dt);

        if (!Config::get().mouseZoomOnPause) return;
        if (Loader::get()->isModLoaded("bobby_shmurner.zoom")) return;
        if (Vars::menuOpen || ImGui::GetIO().WantCaptureMouse) return;

        auto* pl = PlayLayer::get();
        if (!pl || !pl->m_isPaused) return;

        auto mouse = geode::cocos::getMousePos();

#ifdef _WIN32
        if ((GetAsyncKeyState(0x04) & 0x8000) != 0) {
            CCPoint delta = mouse - s_lastMouse;
            pl->setPosition(pl->getPosition() + delta);
        }
#endif
        s_lastMouse = mouse;
    }
};

class $modify(NHPauseZoomMouse, CCMouseDispatcher) {
    bool dispatchScrollMSG(float y, float x) {
        if (Vars::menuOpen || ImGui::GetIO().WantCaptureMouse) {
            return CCMouseDispatcher::dispatchScrollMSG(y, x);
        }

        if (Config::get().mouseZoomOnPause && !Loader::get()->isModLoaded("bobby_shmurner.zoom")) {
            auto* pl = PlayLayer::get();
            if (pl && pl->m_isPaused && CCScene::get()->getChildByID("PauseLayer")) {
                auto mouse = geode::cocos::getMousePos();
                float oldScale = pl->getScale();
                float factor = (y > 0.f) ? 0.9f : 1.1f;
                float newScale = std::clamp(oldScale * factor, 0.2f, 20.0f);

                CCPoint anchor = mouse - (pl->getContentSize() * 0.5f);
                CCPoint diff = pl->getPosition() - anchor;
                pl->setPosition(anchor);
                pl->setScale(newScale);
                pl->setPosition(anchor + diff * (newScale / oldScale));
            }
        }
        return CCMouseDispatcher::dispatchScrollMSG(y, x);
    }
};
