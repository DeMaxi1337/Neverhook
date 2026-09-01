#include <Geode/Geode.hpp>
#include <Geode/modify/PlayerObject.hpp>
#include "../Config.hpp"

using namespace geode::prelude;

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

        if (c.hidePlayer)
            this->setVisible(false);

        if (c.playerOnTop && this->getZOrder() != 1000)
            this->setZOrder(1000);

        if (c.noRobotFire && m_robotFire)
            m_robotFire->setVisible(false);

        if (c.noSwingFire && m_swingFireMiddle)
            m_swingFireMiddle->setVisible(false);

        if (c.noGhostTrail && m_ghostTrail)
            m_ghostTrail->setVisible(false);

        if (c.noTrailBehindWave && m_isDart && m_regularTrail)
            m_regularTrail->setVisible(false);

        if (c.randomSeed) {
            if (auto* pl = PlayLayer::get())
                pl->m_randomSeed = static_cast<uint64_t>(c.randomSeedValue);
        }
    }

    void playSpiderDashEffect(cocos2d::CCPoint from, cocos2d::CCPoint to) {
        if (Config::get().noSpiderDash) return;
        PlayerObject::playSpiderDashEffect(from, to);
    }
};
