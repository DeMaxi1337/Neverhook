#include <Geode/Geode.hpp>
#include <Geode/modify/EditorUI.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <Geode/modify/EditorPauseLayer.hpp>
#include <Geode/modify/EditorOptionsLayer.hpp>
#include <Geode/modify/SliderTouchLogic.hpp>
#include <Geode/modify/GJScaleControl.hpp>
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

class $modify(NHToolboxButtonBypass, EditorOptionsLayer) {
    void onButtonRows(cocos2d::CCObject* sender) {
        if (!Config::get().toolboxButtonBypass) {
            EditorOptionsLayer::onButtonRows(sender);
            return;
        }
        m_buttonRows += sender->getTag() ? 1 : -1;
        m_buttonRows = std::max<int>(1, m_buttonRows);
        m_buttonRowsLabel->setString(fmt::format("{}", m_buttonRows).c_str());
    }

    void onButtonsPerRow(cocos2d::CCObject* sender) {
        if (!Config::get().toolboxButtonBypass) {
            EditorOptionsLayer::onButtonsPerRow(sender);
            return;
        }
        m_buttonsPerRow += sender->getTag() ? 1 : -1;
        m_buttonsPerRow = std::max<int>(1, m_buttonsPerRow);
        m_buttonsPerRowLabel->setString(fmt::format("{}", m_buttonsPerRow).c_str());
    }
};

class $modify(NHSliderLimitBypass, SliderTouchLogic) {
    void ccTouchMoved(cocos2d::CCTouch* touch, cocos2d::CCEvent* event) {
        if (!Config::get().sliderLimitBypass) {
            SliderTouchLogic::ccTouchMoved(touch, event);
            return;
        }

        if (m_thumb->m_vertical)
            m_thumb->setPositionY(this->convertToNodeSpace(touch->getLocation()).y);
        else
            m_thumb->setPositionX(this->convertToNodeSpace(touch->getLocation()).x);

        m_slider->updateBar();
        m_thumb->activate();
    }
};

class $modify(NHSliderScaleControl, GJScaleControl) {
    void ccTouchMoved(cocos2d::CCTouch* touch, cocos2d::CCEvent* event) {
        GJScaleControl::ccTouchMoved(touch, event);

        if (!Config::get().sliderLimitBypass) return;

        if (m_sliderXY && m_sliderXY->m_touchLogic->m_activateThumb) {
            m_sliderXY->getThumb()->setPositionX(
                this->convertToNodeSpace(touch->getLocation()).x);
            m_sliderXY->updateBar();

            float v = m_sliderXY->getThumb()->getValue();
            float value = m_lowerBound + v * (m_upperBound - m_lowerBound);

            updateLabelXY(value);

            if (EditorUI::get())
                EditorUI::get()->scaleXYChanged(value, value, m_scaleLocked);
        }
    }
};
