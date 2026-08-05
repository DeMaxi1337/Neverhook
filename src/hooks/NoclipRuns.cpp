#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/binding/FMODAudioEngine.hpp>
#include <Geode/fmod/fmod.hpp>
#include <filesystem>
#include "../Config.hpp"
#include "../gui/vars.h"

using namespace geode::prelude;

bool nhTrajectoryIsSimulating();

static bool g_nhNoclipEverOn    = false;
static bool g_nhNoclipTurnedOff = false;

bool nhNoclipStatsShouldShow() {
    return g_nhNoclipEverOn && !g_nhNoclipTurnedOff;
}

class $modify(NHNoclipBGL, GJBaseGameLayer) {
    struct Fields {
        bool hasDiedThisAttempt = false;
        int  timeInLevel = 0;
        bool hasDiedThisTick = false;
        bool didDieLastTick = false;
        int  totalTicksDead = 0;
        int  totalDeaths = 0;
    };

    bool nhShouldRegularDie(PlayerObject* pl, GameObject* go) {
        if (!m_isEditor && go == m_anticheatSpike)
            return true;
        if (!pl)
            return true;
        if (m_levelEndAnimationStarted)
            return true;
        return false;
    }

    void nhPlayerDied() {
        auto f = m_fields.self();
        if (!f->hasDiedThisTick) {
            f->hasDiedThisTick = true;
            f->totalTicksDead++;
        }
    }

    void nhPostUpdate() {
        auto f = m_fields.self();

        if (Config::get().noclip)
            g_nhNoclipEverOn = true;
        else if (g_nhNoclipEverOn)
            g_nhNoclipTurnedOff = true;

        if (!f->didDieLastTick && f->hasDiedThisTick) {
            f->totalDeaths++;
            if (Config::get().noclipHitsound) {
                auto fae = FMODAudioEngine::sharedEngine();
                if (fae && fae->m_system) {
                    auto dir = Mod::get()->getResourcesDir();
                    std::string path = (dir / "noclip_hit.ogg").string();
                    if (!std::filesystem::exists(path))
                        path = (dir / "resources" / "noclip_hit.ogg").string();
                    static FMOD::Sound* nhHitSnd = nullptr;
                    if (!nhHitSnd)
                        fae->m_system->createSound(path.c_str(), FMOD_DEFAULT, nullptr, &nhHitSnd);
                    if (nhHitSnd) {
                        FMOD::Channel* ch = nullptr;
                        fae->m_system->playSound(nhHitSnd, nullptr, false, &ch);
                        if (ch) ch->setVolume(Config::get().noclipHitsoundVolume / 100.f);
                    }
                }
            }
        }

        f->didDieLastTick = f->hasDiedThisTick;
        f->hasDiedThisTick = false;

        if (auto pl = typeinfo_cast<PlayLayer*>(this)) {
            if (!pl->m_levelEndAnimationStarted)
                f->timeInLevel = m_gameState.m_currentProgress / kProgressPerFrame;
        } else {
            f->timeInLevel = m_gameState.m_currentProgress / kProgressPerFrame;
        }

        double acc = 100.0;
        if (f->timeInLevel > 0)
            acc = (1.0 - ((double)f->totalTicksDead / (double)f->timeInLevel)) * 100.0;
        if (acc < 0.0) acc = 0.0;

        Vars::noclipAccuracy = acc;
        Vars::noclipDeaths = f->totalDeaths;
    }

    void nhResetValues() {
        auto f = m_fields.self();
        g_nhNoclipEverOn = false;
        g_nhNoclipTurnedOff = false;
        f->hasDiedThisAttempt = false;
        f->timeInLevel = 0;
        f->hasDiedThisTick = false;
        f->didDieLastTick = false;
        f->totalTicksDead = 0;
        f->totalDeaths = 0;
        Vars::noclipAccuracy = 100.0;
        Vars::noclipDeaths = 0;
    }
};

class $modify(NHNoclipPL, PlayLayer) {
    struct Fields {
        CCLayerColor* tintOverlay = nullptr;
    };

    bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
        if (!PlayLayer::init(level, useReplay, dontCreateObjects))
            return false;
        m_fields->tintOverlay = CCLayerColor::create(ccc4(255, 0, 0, 0));
        m_fields->tintOverlay->setID("nh-noclip-tint"_spr);
        addChild(m_fields->tintOverlay, 100);
        return true;
    }

    void destroyPlayer(PlayerObject* p0, GameObject* p1) {

        if (nhTrajectoryIsSimulating())
            return PlayLayer::destroyPlayer(p0, p1);

        if (!Config::get().noclip)
            return PlayLayer::destroyPlayer(p0, p1);

        if (base_cast<NHNoclipBGL*>(this)->nhShouldRegularDie(p0, p1))
            return PlayLayer::destroyPlayer(p0, p1);

        if (!m_started || m_hasCompletedLevel)
            return;

        base_cast<NHNoclipBGL*>(this)->m_fields->hasDiedThisAttempt = true;
        base_cast<NHNoclipBGL*>(this)->nhPlayerDied();

        if (Config::get().noclipTint && m_fields->tintOverlay) {
            float* col = Config::get().noclipTintColor;
            ccColor3B rgb = {
                (GLubyte)(col[0] * 255.f),
                (GLubyte)(col[1] * 255.f),
                (GLubyte)(col[2] * 255.f)
            };
            m_fields->tintOverlay->setColor(rgb);
            m_fields->tintOverlay->stopAllActions();
            m_fields->tintOverlay->setOpacity((GLubyte)(Config::get().noclipTintOpacity / 100.f * 255.f));
            float fade = Config::get().noclipTintTime;
            if (fade < 0.05f) fade = 0.05f;
            m_fields->tintOverlay->runAction(CCFadeTo::create(fade, 0));
        }
    }

    void resetLevel() {
        PlayLayer::resetLevel();
        base_cast<NHNoclipBGL*>(this)->nhResetValues();
        if (m_fields->tintOverlay) {
            m_fields->tintOverlay->stopAllActions();
            m_fields->tintOverlay->setOpacity(0);
        }
    }

    void postUpdate(float dt) {
        PlayLayer::postUpdate(dt);
        base_cast<NHNoclipBGL*>(this)->nhPostUpdate();
    }
};
