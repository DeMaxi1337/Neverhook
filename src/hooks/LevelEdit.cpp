#include <Geode/Geode.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <Geode/modify/EditorPauseLayer.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include "../Config.hpp"

using namespace geode::prelude;

class $modify(NHLevelEditPauseLayer, PauseLayer) {
    void customSetup() {
        if (!Config::get().levelEdit) {
            PauseLayer::customSetup();
            return;
        }
        auto bgl = GJBaseGameLayer::get();
        GJGameLevel* level = bgl ? bgl->m_level : nullptr;
        if (!level) {
            PauseLayer::customSetup();
            return;
        }
        auto lType = level->m_levelType;
        level->m_levelType = GJLevelType::Editor;
        PauseLayer::customSetup();
        level->m_levelType = lType;
    }

    void onTryEdit(cocos2d::CCObject* sender) {
        if (Config::get().levelEdit)
            return PauseLayer::goEdit();
        PauseLayer::onTryEdit(sender);
    }
};

class $modify(NHLevelEditEditorPauseLayer, EditorPauseLayer) {
    void customSetup() {
        if (!Config::get().levelEdit) {
            EditorPauseLayer::customSetup();
            return;
        }
        auto bgl = GJBaseGameLayer::get();
        GJGameLevel* level = bgl ? bgl->m_level : nullptr;
        if (!level) {
            EditorPauseLayer::customSetup();
            return;
        }
        auto lType = level->m_levelType;
        level->m_levelType = GJLevelType::Editor;
        EditorPauseLayer::customSetup();
        level->m_levelType = lType;
    }
};
