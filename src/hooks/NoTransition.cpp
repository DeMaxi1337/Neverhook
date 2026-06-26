#include <Geode/Geode.hpp>
#include <Geode/modify/CCTransitionScene.hpp>
#include "../Config.hpp"

using namespace geode::prelude;

class $modify(NHNoTransition, CCTransitionScene) {
    bool initWithDuration(float t, CCScene* scene) {
        return CCTransitionScene::initWithDuration(
            Config::get().noTransition ? 0.0f : t, scene);
    }
};
