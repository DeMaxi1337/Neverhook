#include <Geode/Geode.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <Geode/modify/EditorPauseLayer.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include "../Config.hpp"

using namespace geode::prelude;

class $modify(NHLevelEditPauseLayer, PauseLayer) {
    void customSetup() {
        auto level = GJBaseGameLayer::get()->m_level;
        auto lType = level->m_levelType;

        if (Config::get().levelEdit)
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
        auto level = GJBaseGameLayer::get()->m_level;
        auto lType = level->m_levelType;

        if (Config::get().levelEdit)
            level->m_levelType = GJLevelType::Editor;

        EditorPauseLayer::customSetup();

        level->m_levelType = lType;
    }
};
