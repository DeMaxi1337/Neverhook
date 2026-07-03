#include <Geode/Geode.hpp>
#include <Geode/modify/PlayerObject.hpp>
#include "../Config.hpp"

using namespace geode::prelude;

// No Dash Fire / Wave Trail Size / No Trail / No Spider Dash.
class $modify(NHCosmeticPlayer, PlayerObject) {
    void update(float dt) {
        PlayerObject::update(dt);

        auto& c = Config::get();

        if (c.noDashFire && m_dashFireSprite)
            m_dashFireSprite->setVisible(false);

        if (c.waveTrailSize && m_waveTrail)
            m_waveTrail->m_waveSize = c.waveTrailSizeValue * m_vehicleSize;

        if (c.noTrail && m_regularTrail)
            m_regularTrail->setVisible(false);
    }

    void playSpiderDashEffect(cocos2d::CCPoint from, cocos2d::CCPoint to) {
        if (Config::get().noSpiderDash) return;
        PlayerObject::playSpiderDashEffect(from, to);
    }
};
