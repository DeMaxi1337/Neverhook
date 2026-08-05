#include <Geode/Geode.hpp>
#include <Geode/modify/ShaderLayer.hpp>
#include "../Config.hpp"

using namespace geode::prelude;

class $modify(NHNoShader, ShaderLayer) {
    void performCalculations() {
        if (Config::get().noShader) {
            m_state.m_usesShaders = false;
            return;
        }
        ShaderLayer::performCalculations();
    }
};
