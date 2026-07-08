#include <Geode/Geode.hpp>
#include <Geode/modify/EditorUI.hpp>
#include "../Config.hpp"

using namespace geode::prelude;

class $modify(NHHideEditorUI, EditorUI) {
    bool init(LevelEditorLayer* editorLayer) {
        if (!EditorUI::init(editorLayer)) return false;
        this->schedule(schedule_selector(NHHideEditorUI::tickVisibility));
        return true;
    }

    void tickVisibility(float) {
        this->setVisible(!Config::get().hideEditorUI);
    }
};
