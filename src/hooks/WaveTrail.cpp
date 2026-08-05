#include <Geode/Geode.hpp>
#include <Geode/modify/HardStreak.hpp>
#include "../Config.hpp"

using namespace geode::prelude;

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
