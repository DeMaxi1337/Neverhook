#include <Geode/Geode.hpp>
#include <Geode/modify/PlayerObject.hpp>
#include <Geode/modify/HardStreak.hpp>
#include <Geode/modify/CCDrawNode.hpp>
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
    }

    void playSpiderDashEffect(cocos2d::CCPoint from, cocos2d::CCPoint to) {
        if (Config::get().noSpiderDash) return;
        PlayerObject::playSpiderDashEffect(from, to);
    }
};

class $modify(NHCosmeticHardStreak, HardStreak) {
    void updateStroke(float dt) {
        auto& c = Config::get();

        if (c.noWavePulse)
            m_pulseSize = 1;

        if (c.solidWaveTrail)
            setBlendFunc({ GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA });

        HardStreak::updateStroke(dt);

        if (c.noWaveTrail)
            this->clear();
    }

    static void onModify(auto& self) {
        (void)self.setHookPriorityPost("HardStreak::updateStroke", Priority::Last);
    }
};

class $modify(NHSolidWaveTrailDraw, cocos2d::CCDrawNode) {
    bool drawPolygon(CCPoint* verts, unsigned int count, const ccColor4F& fillColor,
                     float borderWidth, const ccColor4F& borderColor,
                     cocos2d::BorderAlignment alignment) {
        ccColor4F fill = fillColor;

        if (Config::get().solidWaveTrail && typeinfo_cast<HardStreak*>(this)) {
            fill = ccc4f(
                getColor().r / 255.0f,
                getColor().g / 255.0f,
                getColor().b / 255.0f,
                getOpacity() / 255.0f
            );
            setBlendFunc({ GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA });
        }

        return CCDrawNode::drawPolygon(verts, count, fill, borderWidth, borderColor, alignment);
    }
};
