#pragma once

#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/UILayer.hpp>
#include <imgui.h>
#include <chrono>
#include <vector>
#include <string>
#include <ctime>
#include <algorithm>

#include "vars.h"
#include "../hooks/MacroEngine.hpp"

namespace nh::status {

inline auto g_sessionStart = std::chrono::steady_clock::now();
inline std::vector<std::chrono::steady_clock::time_point> g_clicksP1;
inline std::vector<std::chrono::steady_clock::time_point> g_clicksP2;

inline float g_bestStart = 0.f;
inline float g_bestEnd = 0.f;

inline void recordClick(bool player1) {
    auto now = std::chrono::steady_clock::now();
    if (player1)
        g_clicksP1.push_back(now);
    else
        g_clicksP2.push_back(now);
}

inline void cleanOldClicks() {
    auto now = std::chrono::steady_clock::now();
    while (!g_clicksP1.empty() && std::chrono::duration<float>(now - g_clicksP1.front()).count() > 1.0f)
        g_clicksP1.erase(g_clicksP1.begin());
    while (!g_clicksP2.empty() && std::chrono::duration<float>(now - g_clicksP2.front()).count() > 1.0f)
        g_clicksP2.erase(g_clicksP2.begin());
}

inline bool isGameCheated() {
    if (Vars::noclip) return true;
    if (Vars::speedhack && std::abs(Vars::speedhackValue - 1.0f) > 0.005f) return true;
    if (Vars::frameAdvance) return true;
    if (Vars::jumpHack) return true;
    if (Vars::instantComplete) return true;
    if (Vars::allModesPlatformer) return true;
    if (Vars::hitboxMultiplier && (std::abs(Vars::hitboxMultPlayer - 1.0f) > 0.01f ||
                                   std::abs(Vars::hitboxMultSolid - 1.0f) > 0.01f ||
                                   std::abs(Vars::hitboxMultHazard - 1.0f) > 0.01f)) return true;
    if (nh::MacroEngine::get().isPlaying()) return true;
    return false;
}

inline const char* getFontFile() {
    switch (Vars::statusFont) {
    case 1:  return "chatFont.fnt";
    case 2:  return "goldFont.fnt";
    default: return "bigFont.fnt";
    }
}

struct StatusEntry {
    std::string text;
    cocos2d::ccColor3B color;
    bool isDot = false;
};

}

class $modify(NHStatusBGL, ::GJBaseGameLayer) {
    void handleButton(bool down, int button, bool player1) {
        if (down)
            nh::status::recordClick(player1);
        GJBaseGameLayer::handleButton(down, button, player1);
    }
};

class $modify(NHStatusPlayLayer, ::PlayLayer) {
    bool init(::GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
        if (!PlayLayer::init(level, useReplay, dontCreateObjects)) return false;
        nh::status::g_bestStart = PlayLayer::getCurrentPercent();
        nh::status::g_bestEnd = PlayLayer::getCurrentPercent();
        return true;
    }

    void resetLevel() {
        PlayLayer::resetLevel();
        nh::status::g_bestStart = PlayLayer::getCurrentPercent();
    }
};

class $modify(NHStatusUILayer, ::UILayer) {
    struct Fields {
        cocos2d::CCNode* m_statusContainer = nullptr;
    };

    bool init(::GJBaseGameLayer* bgl) {
        if (!UILayer::init(bgl)) return false;

        m_fields->m_statusContainer = cocos2d::CCNode::create();
        m_fields->m_statusContainer->setID("nh-status-container"_spr);
        this->addChild(m_fields->m_statusContainer, 1000);

        this->schedule(schedule_selector(NHStatusUILayer::updateStatusLabels));
        return true;
    }

    void updateStatusLabels(float dt) {
        if (!m_fields->m_statusContainer) return;
        m_fields->m_statusContainer->removeAllChildren();

        if (Vars::hideStatus) return;

        auto game = ::GJBaseGameLayer::get();
        if (!game) return;
        auto playLayer = ::PlayLayer::get();

        nh::status::cleanOldClicks();

        const float scale = Vars::statusScale;
        const GLubyte opacity = (GLubyte)std::clamp((int)(Vars::statusOpacity * 2.55f), 0, 255);
        const bool cheated = nh::status::isGameCheated();
        const char* font = nh::status::getFontFile();

        std::vector<nh::status::StatusEntry> groups[7];

        auto add = [&](int pos, const std::string& txt, cocos2d::ccColor3B col, bool isDot = false) {
            if (pos >= 1 && pos <= 6 && !txt.empty()) {
                groups[pos].push_back({ txt, col, isDot });
            }
        };

        const cocos2d::ccColor3B white = { 255, 255, 255 };

        for (int id : Vars::statusOrder) {
            switch (id) {
            case 0:
                if (Vars::statusCheatIndicator > 0) {
                    cocos2d::ccColor3B col = cheated ? cocos2d::ccColor3B{ 255, 60, 60 } : cocos2d::ccColor3B{ 60, 255, 60 };
                    if (Vars::statusCheatIndicatorMode == 0) {
                        add(Vars::statusCheatIndicator, ".", col, true);
                    } else {
                        add(Vars::statusCheatIndicator, cheated ? "Cheated" : "Legit", col, false);
                    }
                }
                break;
            case 1:
                if (Vars::statusFps > 0) {
                    char buf[32];
                    snprintf(buf, sizeof(buf), "%.0f FPS", ImGui::GetIO().Framerate);
                    add(Vars::statusFps, buf, white);
                }
                break;
            case 2:
                if (Vars::statusCps > 0) {
                    char buf[32];
                    if (game->m_gameState.m_isDualMode)
                        snprintf(buf, sizeof(buf), "%zu | %zu CPS", nh::status::g_clicksP1.size(), nh::status::g_clicksP2.size());
                    else
                        snprintf(buf, sizeof(buf), "%zu CPS", nh::status::g_clicksP1.size());
                    add(Vars::statusCps, buf, white);
                }
                break;
            case 3:
                if (Vars::statusBestRun > 0 && playLayer) {
                    float cur = playLayer->getCurrentPercent();
                    if (cur > nh::status::g_bestEnd)
                        nh::status::g_bestEnd = cur;
                    char buf[48];
                    snprintf(buf, sizeof(buf), "Best: %.0f%% - %.0f%%", nh::status::g_bestStart, nh::status::g_bestEnd);
                    add(Vars::statusBestRun, buf, white);
                }
                break;
            case 4:
                if (Vars::statusNoclipAcc > 0 && playLayer) {
                    char buf[32];
                    snprintf(buf, sizeof(buf), "Acc: 100.00%%");
                    add(Vars::statusNoclipAcc, buf, white);
                }
                break;
            case 5:
                if (Vars::statusNoclipDeaths > 0 && playLayer) {
                    char buf[32];
                    snprintf(buf, sizeof(buf), "Deaths: 0");
                    add(Vars::statusNoclipDeaths, buf, white);
                }
                break;
            case 6:
                if (Vars::statusAttempts > 0 && playLayer && playLayer->m_level) {
                    char buf[32];
                    snprintf(buf, sizeof(buf), "Attempt %d", (int)playLayer->m_level->m_attempts);
                    add(Vars::statusAttempts, buf, white);
                }
                break;
            case 7:
                if (Vars::statusJumps > 0 && playLayer) {
                    char buf[32];
                    snprintf(buf, sizeof(buf), "Jumps: %d", playLayer->m_jumps);
                    add(Vars::statusJumps, buf, white);
                }
                break;
            case 8:
                if (Vars::statusPercentage > 0 && playLayer) {
                    char buf[32];
                    snprintf(buf, sizeof(buf), "%.2f%%", playLayer->getCurrentPercent());
                    add(Vars::statusPercentage, buf, white);
                }
                break;
            case 9:
                if (Vars::statusLevelTime > 0 && playLayer) {
                    int totalSec = (int)playLayer->m_attemptTime;
                    int m = totalSec / 60;
                    int s = totalSec % 60;
                    char buf[32];
                    snprintf(buf, sizeof(buf), "Time: %02d:%02d", m, s);
                    add(Vars::statusLevelTime, buf, white);
                }
                break;
            case 10:
                if (Vars::statusSessionTime > 0) {
                    auto now = std::chrono::steady_clock::now();
                    int totalSec = (int)std::chrono::duration<float>(now - nh::status::g_sessionStart).count();
                    int h = totalSec / 3600;
                    int m = (totalSec % 3600) / 60;
                    int s = totalSec % 60;
                    char buf[32];
                    if (h > 0)
                        snprintf(buf, sizeof(buf), "Session: %02d:%02d:%02d", h, m, s);
                    else
                        snprintf(buf, sizeof(buf), "Session: %02d:%02d", m, s);
                    add(Vars::statusSessionTime, buf, white);
                }
                break;
            case 11:
                if (Vars::statusClock > 0) {
                    auto now = std::time(nullptr);
                    auto* tm = std::localtime(&now);
                    char buf[32];
                    if (tm)
                        snprintf(buf, sizeof(buf), "%02d:%02d:%02d", tm->tm_hour, tm->tm_min, tm->tm_sec);
                    else
                        snprintf(buf, sizeof(buf), "00:00:00");
                    add(Vars::statusClock, buf, white);
                }
                break;
            case 12:
                if (Vars::statusFrameCounter > 0 && playLayer) {
                    char buf[32];
                    snprintf(buf, sizeof(buf), "Frame: %d", playLayer->m_gameState.m_currentProgress);
                    add(Vars::statusFrameCounter, buf, white);
                }
                break;
            case 13:
                if (Vars::statusPosition > 0 && game->m_player1) {
                    char buf[48];
                    snprintf(buf, sizeof(buf), "X: %.1f  Y: %.1f", game->m_player1->getPositionX(), game->m_player1->getPositionY());
                    add(Vars::statusPosition, buf, white);
                }
                break;
            case 14:
                if (Vars::statusVelocity > 0 && game->m_player1) {
                    char buf[48];
                    snprintf(buf, sizeof(buf), "Vel: %.1f", game->m_player1->m_yVelocity);
                    add(Vars::statusVelocity, buf, white);
                }
                break;
            case 15:
                if (Vars::statusMessage > 0 && !Vars::statusMessageText.empty()) {
                    add(Vars::statusMessage, Vars::statusMessageText, white);
                }
                break;
            case 16:
                if (Vars::statusTestmode > 0 && playLayer && playLayer->m_isTestMode) {
                    add(Vars::statusTestmode, "Testmode", cocos2d::ccColor3B{ 255, 220, 50 });
                }
                break;
            case 17:
                if (Vars::statusReplayState > 0) {
                    auto& eng = nh::MacroEngine::get();
                    if (eng.isRecording())
                        add(Vars::statusReplayState, "[REC]", cocos2d::ccColor3B{ 255, 60, 60 });
                    else if (eng.isPlaying())
                        add(Vars::statusReplayState, "[PLAY]", cocos2d::ccColor3B{ 60, 255, 60 });
                }
                break;
            }
        }

        const cocos2d::CCSize winSize = cocos2d::CCDirector::sharedDirector()->getWinSize();
        const float margin = 4.f;
        const float lineSpacing = 32.f * scale;

        for (int p = 1; p <= 6; ++p) {
            const auto& lines = groups[p];
            if (lines.empty()) continue;

            for (size_t i = 0; i < lines.size(); ++i) {
                const auto& entry = lines[i];
                auto* label = cocos2d::CCLabelBMFont::create(entry.text.c_str(), font);
                if (!label) continue;

                float itemScale = entry.isDot ? (scale * 2.4f) : scale;
                label->setScale(itemScale);
                label->setColor(entry.color);
                label->setOpacity(opacity);

                float dotYOffset = entry.isDot ? (27.f * scale) : 0.f;
                float dotXOffset = entry.isDot ? (2.f * scale) : 0.f;

                switch (p) {
                case 1:
                    label->setAnchorPoint({ 0.f, 1.f });
                    label->setPosition({ margin + dotXOffset, winSize.height - margin - (i * lineSpacing) + dotYOffset });
                    break;
                case 2:
                    label->setAnchorPoint({ 1.f, 1.f });
                    label->setPosition({ winSize.width - margin - dotXOffset, winSize.height - margin - (i * lineSpacing) + dotYOffset });
                    break;
                case 3:
                    label->setAnchorPoint({ 0.f, 0.f });
                    label->setPosition({ margin + dotXOffset, margin + (i * lineSpacing) + dotYOffset });
                    break;
                case 4:
                    label->setAnchorPoint({ 1.f, 0.f });
                    label->setPosition({ winSize.width - margin - dotXOffset, margin + (i * lineSpacing) + dotYOffset });
                    break;
                case 5:
                    label->setAnchorPoint({ 0.5f, 1.f });
                    label->setPosition({ winSize.width * 0.5f, winSize.height - margin - (i * lineSpacing) + dotYOffset });
                    break;
                case 6:
                    label->setAnchorPoint({ 0.5f, 0.f });
                    label->setPosition({ winSize.width * 0.5f, margin + (i * lineSpacing) + dotYOffset });
                    break;
                }

                m_fields->m_statusContainer->addChild(label);
            }
        }
    }
};
