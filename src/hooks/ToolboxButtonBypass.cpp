#include <Geode/Geode.hpp>
#include <Geode/modify/EditorOptionsLayer.hpp>
#include "../Config.hpp"

using namespace geode::prelude;

// Toolbox Button Bypass -- unlock the editor button-row limits.
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
