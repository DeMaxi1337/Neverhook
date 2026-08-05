#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include "../Config.hpp"

#include <algorithm>
#include <cmath>

using namespace geode::prelude;

namespace {
    struct Track {
        CCPoint p0 = CCPointZero;
        CCPoint p1 = CCPointZero;
        CCPoint p2 = CCPointZero;
        int samples = 0;
    };

    struct PlayerTrack : Track {
        float olderAngle = 0.f;
        float newerAngle = 0.f;
    };

    void advance(Track& track, CCPoint sample) {
        if (track.samples == 0) {
            track.p0 = track.p1 = track.p2 = sample;
            track.samples = 1;
            return;
        }
        track.p0 = track.p1;
        track.p1 = track.p2;
        track.p2 = sample;
        if (track.samples < 3)
            track.samples++;
    }

    float projectAxis(float p0, float p1, float p2, float t, bool haveThree) {
        float velocity = p2 - p1;
        if (!haveThree)
            return p2 + velocity * t;

        float previousVelocity = p1 - p0;
        float acceleration = velocity - previousVelocity;

        if (std::fabs(acceleration) > 2.0f * (std::fabs(velocity) + 0.0001f))
            return p2 + velocity * t;

        return p2 + velocity * t + 0.5f * acceleration * t * t;
    }

    CCPoint project(Track const& track, float progress, int ticksBundled) {
        float t = progress / static_cast<float>(std::max(1, ticksBundled));
        bool haveThree = track.samples >= 3;
        return {
            projectAxis(track.p0.x, track.p1.x, track.p2.x, t, haveThree),
            projectAxis(track.p0.y, track.p1.y, track.p2.y, t, haveThree)
        };
    }

    bool renderable(Track const& track) {
        return track.samples >= 2;
    }

    float shortestAngle(float from, float to) {
        return std::remainder(to - from, 360.f);
    }
}

class $modify(NHFrameExtrapolation, GJBaseGameLayer) {
    static void onModify(auto& self) {

        (void)self.setHookPriorityPre("GJBaseGameLayer::update", Priority::First);
    }

    struct Fields {
        int ticksThisFrame = 0;

        PlayerTrack playerOne;
        PlayerTrack playerTwo;
        Track world;
        Track groundOne;
        Track groundTwo;
    };

    double tickSeconds() const {
        double tps = Config::get().tpsBypass
            ? std::clamp(static_cast<double>(Config::get().tpsValue), 1.0, 100000.0)
            : 240.0;
        return 1.0 / tps;
    }

    double getModifiedDelta(float dt) {
        double consumed = GJBaseGameLayer::getModifiedDelta(dt);
        if (consumed > 0.0) {
            double per = tickSeconds();
            int ticks = per > 0.0
                ? static_cast<int>(std::lround(consumed / per))
                : 1;
            m_fields->ticksThisFrame += std::max(1, ticks);
        }
        return consumed;
    }

    void update(float dt) {
        auto state = m_fields.self();
        auto& cfg = Config::get();

        bool usable = cfg.frameExtrapolation && !cfg.frameAdvance && dt > 0.f;
        auto play = PlayLayer::get();

        if (!usable || !play || m_isEditor || play->m_levelEndAnimationStarted) {
            forgetTracks();
            GJBaseGameLayer::update(dt);
            return;
        }

        state->ticksThisFrame = 0;
        GJBaseGameLayer::update(dt);

        if (state->ticksThisFrame > 0)
            sampleTracks();

        if (!m_started || m_playerDied || isFlipping() ||
            play->m_levelEndAnimationStarted) {
            return;
        }

        double per = tickSeconds();
        if (per <= 0.0)
            return;

        float progress = static_cast<float>(std::clamp(
            static_cast<double>(m_extraDelta) / per, 0.0, 1.0
        ));

        int bundle = std::max(1, state->ticksThisFrame);
        drawPrediction(progress, bundle);
    }

    CCSpriteBatchNode* groundBatch(GJGroundLayer* ground) const {
        return ground ? ground->getChildByType<CCSpriteBatchNode>(0) : nullptr;
    }

    void forgetTracks() {
        auto state = m_fields.self();
        state->playerOne.samples = 0;
        state->playerTwo.samples = 0;
        state->world.samples = 0;
        state->groundOne.samples = 0;
        state->groundTwo.samples = 0;
        state->ticksThisFrame = 0;
    }

    void samplePlayer(PlayerTrack& track, PlayerObject* player) {
        if (!player)
            return;

        float angle = player->m_mainLayer ? player->m_mainLayer->getRotation() : 0.f;

        if (track.samples == 0)
            track.olderAngle = angle;
        else
            track.olderAngle = track.newerAngle;

        track.newerAngle = angle;
        advance(track, player->getPosition());
    }

    void sampleTracks() {
        auto state = m_fields.self();

        samplePlayer(state->playerOne, m_player1);
        samplePlayer(state->playerTwo, m_player2);

        if (m_objectLayer)
            advance(state->world, m_objectLayer->getPosition());
        if (auto batch = groundBatch(m_groundLayer))
            advance(state->groundOne, batch->getPosition());
        if (auto batch = groundBatch(m_groundLayer2))
            advance(state->groundTwo, batch->getPosition());
    }

    void drawPlayer(PlayerTrack const& track, PlayerObject* player,
                    float progress, int bundle) {
        if (!renderable(track) || !player)
            return;

        player->CCNode::setPosition(project(track, progress, bundle));

        if (player->m_mainLayer) {
            float step = shortestAngle(track.olderAngle, track.newerAngle)
                / static_cast<float>(bundle);
            player->m_mainLayer->setRotation(track.newerAngle + step * progress);
        }
    }

    void drawNode(Track const& track, CCNode* node, float progress, int bundle) {
        if (renderable(track) && node)
            node->setPosition(project(track, progress, bundle));
    }

    void drawPrediction(float progress, int bundle) {
        auto state = m_fields.self();
        drawPlayer(state->playerOne, m_player1, progress, bundle);
        drawPlayer(state->playerTwo, m_player2, progress, bundle);
        drawNode(state->world, m_objectLayer, progress, bundle);
        drawNode(state->groundOne, groundBatch(m_groundLayer), progress, bundle);
        drawNode(state->groundTwo, groundBatch(m_groundLayer2), progress, bundle);
    }
};
