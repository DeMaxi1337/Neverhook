/* 
THIS SHIT DOESNT WORK PROPERLY, IM FUCKING SICK OF CODING THAT MODULE, GODDAMMIT
03.07.2026 23:58
*/

/*
#include <Geode/Geode.hpp>
#include <Geode/modify/LevelCell.hpp>
#include <Geode/modify/BoomListView.hpp>
#include "../Config.hpp"

using namespace geode::prelude;

static constexpr float NH_COMPACT_CELL_HEIGHT = 60.f;

class $modify(NHCompactLevelCell, LevelCell) {
    void loadFromLevel(GJGameLevel* level) {
        if (Config::get().compactLists) m_compactView = true;
        LevelCell::loadFromLevel(level);
    }
    void loadCustomLevelCell() {
        if (Config::get().compactLists) m_compactView = true;
        LevelCell::loadCustomLevelCell();
    }
    void loadLocalLevelCell() {
        if (Config::get().compactLists) m_compactView = true;
        LevelCell::loadLocalLevelCell();
    }
};

class $modify(NHCompactListView, BoomListView) {
    float cellHeightForRowAtIndexPath(CCIndexPath& indexPath, TableView* tableView) {
        if (Config::get().compactLists && m_type == BoomListType::Default)
            return NH_COMPACT_CELL_HEIGHT;
        return BoomListView::cellHeightForRowAtIndexPath(indexPath, tableView);
    }
};
*/
