#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/UILayer.hpp>
#include <algorithm>
#include <vector>
#include "../Config.hpp"

using namespace geode::prelude;

class $modify(NHStartposSwitcher, PlayLayer) {
    struct Fields {
        std::vector<StartPosObject*> spots;
        int index = 0;
        cocos2d::CCLabelBMFont* label = nullptr;
    };

    void nhUpdateLabel() {
        if (m_fields->label)
            m_fields->label->setCString(
                fmt::format("{}/{}", m_fields->index, (int)m_fields->spots.size()).c_str());
    }

    void createObjectsFromSetupFinished() {
        PlayLayer::createObjectsFromSetupFinished();

        m_fields->spots.clear();
        if (m_objects) {
            for (auto obj : CCArrayExt<GameObject*>(m_objects)) {
                if (auto sp = typeinfo_cast<StartPosObject*>(obj))
                    m_fields->spots.push_back(sp);
            }
        }
        std::sort(m_fields->spots.begin(), m_fields->spots.end(),
            [](StartPosObject* a, StartPosObject* b) { return a->m_positionX < b->m_positionX; });

        m_fields->index = 0;
        if (m_startPosObject) {
            for (int i = 0; i < (int)m_fields->spots.size(); i++) {
                if (m_fields->spots[i] == m_startPosObject) {
                    m_fields->index = i + 1;
                    break;
                }
            }
        }

        if (m_fields->label) {
            m_fields->label->removeFromParent();
            m_fields->label = nullptr;
        }

        if (Config::get().startposSwitcher && !m_fields->spots.empty() && m_uiLayer) {
            auto win = cocos2d::CCDirector::sharedDirector()->getWinSize();
            auto label = cocos2d::CCLabelBMFont::create(
                fmt::format("{}/{}", m_fields->index, (int)m_fields->spots.size()).c_str(),
                "bigFont.fnt");
            label->setScale(0.5f);
            label->setPosition(win.width / 2.f, 20.f);
            label->setOpacity(120);
            label->setZOrder(999);
            label->setID("nhStartposIndicator"_spr);
            m_uiLayer->addChild(label);
            m_fields->label = label;
        }
    }

    void nhSwitchStartpos(int delta) {
        auto& spots = m_fields->spots;
        if (spots.empty()) return;

        int count = (int)spots.size();
        int next = m_fields->index + delta;
        if (next < 0) next = count;
        if (next > count) next = 0;
        m_fields->index = next;

        m_isTestMode = (next != 0);
        m_currentCheckpoint = nullptr;
        setStartPosObject(next == 0 ? nullptr : spots[next - 1]);

        if (m_isPracticeMode)
            resetLevelFromStart();
        else
            PlayLayer::resetLevel();
        startMusic();

        nhUpdateLabel();
    }
};

class $modify(NHStartposSwitcherUI, UILayer) {
#if GEODE_COMP_GD_VERSION >= 22081
    void handleKeypress(cocos2d::enumKeyCodes key, bool down, double timestamp) {
#else
    void handleKeypress(cocos2d::enumKeyCodes key, bool down) {
#endif
        if (down && Config::get().startposSwitcher) {
            if (auto pl = static_cast<NHStartposSwitcher*>(PlayLayer::get())) {
                if (!pl->m_levelEndAnimationStarted) {
                    if (key == cocos2d::enumKeyCodes::KEY_Q)
                        pl->nhSwitchStartpos(-1);
                    else if (key == cocos2d::enumKeyCodes::KEY_E)
                        pl->nhSwitchStartpos(1);
                }
            }
        }

#if GEODE_COMP_GD_VERSION >= 22081
        UILayer::handleKeypress(key, down, timestamp);
#else
        UILayer::handleKeypress(key, down);
#endif
    }
};
