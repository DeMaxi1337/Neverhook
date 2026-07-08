#include <Geode/Geode.hpp>
#include <Geode/modify/EditorUI.hpp>
#include "../Config.hpp"

using namespace geode::prelude;

class $modify(NHNoZoomLimit, EditorUI) {
    void zoomNoLimit(bool zoomingIn) {
        float scale = m_editorLayer->m_groundLayer->getScale();
        scale += zoomingIn ? 0.1f : -0.1f;
        scale = std::max<float>(scale, 0.1f);
        updateZoom(scale);
    }

    void zoomIn(cocos2d::CCObject* sender) {
        if (Config::get().noZoomLimit) return zoomNoLimit(true);
        EditorUI::zoomIn(sender);
    }

    void zoomOut(cocos2d::CCObject* sender) {
        if (Config::get().noZoomLimit) return zoomNoLimit(false);
        EditorUI::zoomOut(sender);
    }

    void scrollWheel(float y, float x) {
        auto scale = m_editorLayer->m_groundLayer->getScale();

        EditorUI::scrollWheel(y, x);

        if (Config::get().noZoomLimit
                && m_editorLayer->m_playbackMode != PlaybackMode::Playing
                && CCKeyboardDispatcher::get()->getControlKeyPressed()) {
            m_editorLayer->m_groundLayer->setScale(scale);
            (y < 0 || x < 0) ? zoomNoLimit(true) : zoomNoLimit(false);
        }
    }
};
