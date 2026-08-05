#include <Geode/Geode.hpp>
#include <Geode/modify/SliderTouchLogic.hpp>
#include <Geode/modify/GJScaleControl.hpp>
#include "../Config.hpp"

using namespace geode::prelude;

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
