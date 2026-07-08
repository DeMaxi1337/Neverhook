#include <Geode/Geode.hpp>
#include <Geode/modify/EditorUI.hpp>
#include "../Config.hpp"

using namespace geode::prelude;

class $modify(NHNoCustomObjLimit, EditorUI) {
    void onNewCustomItem(cocos2d::CCObject* sender) {
        if (!Config::get().noCustomObjLimit) {
            EditorUI::onNewCustomItem(sender);
            return;
        }

        CCArray* objs = m_selectedObjects;
        if (m_selectedObjects->count() == 0) {
            objs = CCArray::create();
            objs->addObject(m_selectedObject);
        }

        GameManager::get()->addNewCustomObject(copyObjects(objs, false, false));
        m_selectedObjectIndex = 0;
        reloadCustomItems();
    }
};
