#include <Geode/Geode.hpp>
#include <Geode/modify/CustomListView.hpp>
#include <Geode/modify/LevelCell.hpp>
#include "../Config.hpp"

using namespace geode::prelude;

class $modify(NHCompactListView, CustomListView) {
    static CustomListView* create(
        cocos2d::CCArray* items,
        TableViewCellDelegate* delegate,
        float width,
        float height,
        int unused,
        BoomListType type,
        float unused2
    ) {
        if (Config::get().compactList && type == BoomListType::Level)
            type = BoomListType::Level4;

        return CustomListView::create(
            items, delegate, width, height, unused, type, unused2
        );
    }
};

class $modify(NHCompactLevelCell, LevelCell) {
    static void onModify(auto& self) {
        if (!self.setHookPriority("LevelCell::loadCustomLevelCell", Priority::EarlyPost))
            log::warn("[Neverhook] couldn't set LevelCell hook priority; compact list may look off");
    }

    void loadCustomLevelCell() {
        LevelCell::loadCustomLevelCell();

        if (!Config::get().compactList || !m_compactView) return;
        if (m_level && m_level->m_listPosition != 0) return; // keep real rankings
        if (!m_mainLayer) return;

        auto place = m_mainLayer->getChildByID("level-place");
        if (!place || !place->isVisible()) return;

        place->setVisible(false);
        auto pos = place->getPosition();

        if (auto pct = m_mainLayer->getChildByID("percentage-label"))
            pct->setPosition(pos);
        if (auto chk = m_mainLayer->getChildByID("completed-icon"))
            chk->setPosition(pos);
    }
};
