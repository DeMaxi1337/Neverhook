#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include "../Config.hpp"

using namespace geode::prelude;

class $modify(NHJumpHack, GJBaseGameLayer) {
    void update(float dt) {
        if (Config::get().jumpHack) {
            if (m_player1) m_player1->m_isOnGround = true;
            if (m_player2) m_player2->m_isOnGround = true;
        }
        GJBaseGameLayer::update(dt);
    }
};
