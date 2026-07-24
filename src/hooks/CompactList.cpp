#include <Geode/Geode.hpp>
#include <Geode/modify/CustomListView.hpp>
#include <Geode/modify/LevelCell.hpp>
#include "../Config.hpp"

using namespace geode::prelude;

// Compact List: shrinks the tall level rows so more levels fit on screen.
//
// In GD 2.2081 the row height is decided by the list "type" (BoomListType) at
// creation time. Online / Saved / search level rows use BoomListType::Level, and
// Level4 is the built-in COMPACT version of that row. So we intercept
// CustomListView::create and remap Level -> Level4 when the toggle is on.
//
// NOTE: the editor "My Levels" list uses a different type. GD has NO working
// compact layout for editor cells -- forcing them compact breaks the View button
// (it stops receiving taps), so we deliberately DO NOT touch that type here.
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

// Replace the far-left "0" that shows up on every compact row with the level's
// own completion indicator (percentage, or the green completed checkmark).
//
// That "0" is GD's "level-place" label: on a compact row GD reserves a left
// column for the level's position in a RANKED list. Lists that aren't ranked
// (Saved Levels, search results, etc.) report position 0, so the label just
// prints "0". We hide it there and move the completion percentage / checkmark
// into that spot. Genuinely ranked lists (position != 0) keep their real number.
class $modify(NHCompactLevelCell, LevelCell) {
    static void onModify(auto& self) {
        // Run after GD (and the node-ids mod) have finished building the cell,
        // so the child nodes and their IDs exist when we look them up.
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
