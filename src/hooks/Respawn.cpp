#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include "../Config.hpp"

using namespace geode::prelude;

class $modify(NHRespawn, PlayLayer) {
    void destroyPlayer(PlayerObject* player, GameObject* object) {
        PlayLayer::destroyPlayer(player, object);

        if (!player)
            return;

        auto& c = Config::get();
        if (!c.instantRestart && !c.customRespawn)
            return;

        if (this->getActionByTag(0x10)) {
            this->stopActionByTag(0x10);

            float delay = (!c.instantRestart && c.customRespawn)
                ? std::max(0.f, c.respawnTime)
                : 0.f;

            auto seq = CCSequence::create(
                CCDelayTime::create(delay),
                CCCallFunc::create(this, callfunc_selector(PlayLayer::delayedResetLevel)),
                nullptr);
            seq->setTag(0x10);
            this->runAction(seq);
        }
    }
};
