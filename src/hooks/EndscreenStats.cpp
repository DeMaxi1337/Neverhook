#include <Geode/Geode.hpp>
#include <Geode/modify/EndLevelLayer.hpp>

#include <algorithm>
#include <chrono>
#include <string>
#include <vector>

#include "EndscreenPhrases.hpp"
#include "../Config.hpp"
#include "../gui/vars.h"

using namespace geode::prelude;

bool nhNoclipStatsShouldShow();

namespace {
    constexpr float kLineStep    = 27.f;
    constexpr float kStatScale   = 0.82f;
    constexpr float kColGap      = 46.f;
    constexpr float kMsgGap      = 42.f;
    constexpr float kMsgScale    = 0.62f;
    constexpr float kMaxWidth    = 300.f;
    constexpr float kBlockOffset = -26.f;
}

static std::string nhFormatTime(double totalSeconds) {
    if (totalSeconds < 0.0)
        totalSeconds = 0.0;

    std::chrono::milliseconds duration(static_cast<long long>(totalSeconds * 1000.0));

    auto hours = std::chrono::duration_cast<std::chrono::hours>(duration);
    duration -= hours;
    auto minutes = std::chrono::duration_cast<std::chrono::minutes>(duration);
    duration -= minutes;
    auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration);

    if (hours.count() > 0) {
        return fmt::format("Time: {:02}:{:02}:{:02}",
            hours.count(), minutes.count(), seconds.count());
    }

    return fmt::format("Time: {:02}:{:02}", minutes.count(), seconds.count());
}

static bool nhIsVanillaStatLabel(const std::string& t) {
    static const char* kPrefixes[] = {
        "Attempts", "Attempt", "Jumps", "Jump", "Time", "Points"
    };

    for (const char* p : kPrefixes) {
        if (t.rfind(p, 0) == 0)
            return true;
    }
    return false;
}

static void nhCollectLabels(CCNode* node, std::vector<CCLabelBMFont*>& out, int depth = 0) {
    if (!node || depth > 8)
        return;

    auto children = node->getChildren();
    if (!children)
        return;

    for (auto child : CCArrayExt<CCNode*>(children)) {
        if (!child)
            continue;

        if (auto lbl = typeinfo_cast<CCLabelBMFont*>(child)) {
            out.push_back(lbl);
            continue;
        }

        if (typeinfo_cast<CCMenu*>(child))
            continue;

        nhCollectLabels(child, out, depth + 1);
    }
}

static float nhBuildColumn(const std::vector<std::string>& texts,
                           std::vector<CCLabelBMFont*>& out) {
    float widest = 0.f;

    for (const auto& text : texts) {
        auto lbl = CCLabelBMFont::create(text.c_str(), "goldFont.fnt");
        lbl->setScale(kStatScale);
        widest = std::max(widest, lbl->getScaledContentSize().width);
        out.push_back(lbl);
    }

    return widest;
}

static void nhPlaceColumn(CCNode* container,
                          const std::vector<CCLabelBMFont*>& labels,
                          size_t rows, float axisX) {
    const float off = (static_cast<float>(rows) - static_cast<float>(labels.size())) / 2.f;

    for (size_t i = 0; i < labels.size(); ++i) {
        auto lbl = labels[i];
        lbl->setAnchorPoint(ccp(0.5f, 0.5f));
        lbl->setPosition(ccp(axisX, -(off + static_cast<float>(i)) * kLineStep));
        container->addChild(lbl);
    }
}

class $modify(NHEndLevelLayer, EndLevelLayer) {

    void customSetup() {
        EndLevelLayer::customSetup();

        if (!Config::get().endscreenStats)
            return;

        auto bgl = GJBaseGameLayer::get();
        if (!bgl || !m_mainLayer)
            return;

        if (auto old = m_mainLayer->getChildByID("nh-endscreen-stats"_spr))
            old->removeFromParent();

        const bool isPlatformer = bgl->m_level && bgl->m_level->isPlatformer();
        const bool showNoclip   = nhNoclipStatsShouldShow();

        std::string systemMsg;

        bool  haveAnchor = false;
        float anchorX = 0.f, statTopY = 0.f, statBotY = 0.f;

        std::vector<CCLabelBMFont*> labels;
        nhCollectLabels(m_mainLayer, labels);

        for (auto lbl : labels) {
            std::string text = lbl->getString();

            if (nh::phrases::isReservedMessage(text))
                systemMsg = text;

            if (nhIsVanillaStatLabel(text)) {
                if (auto parent = lbl->getParent()) {
                    CCPoint world = parent->convertToWorldSpace(lbl->getPosition());
                    CCPoint local = m_mainLayer->convertToNodeSpace(world);

                    if (!haveAnchor) {
                        haveAnchor = true;
                        anchorX  = local.x;
                        statTopY = statBotY = local.y;
                    }
                    else {
                        if (local.y > statTopY) statTopY = local.y;
                        if (local.y < statBotY) statBotY = local.y;
                    }
                }
            }

            lbl->setVisible(false);
        }

        std::vector<std::string> leftText;
        std::vector<std::string> rightText;

        if (isPlatformer) {

            leftText.push_back(nhFormatTime(bgl->m_gameState.m_totalTime));
        }
        else {
            leftText.push_back(fmt::format("Attempts: {}", bgl->m_attempts));

            if (auto pl = typeinfo_cast<PlayLayer*>(bgl))
                leftText.push_back(fmt::format("Jumps: {}", pl->m_jumps));

            leftText.push_back(nhFormatTime(bgl->m_gameState.m_totalTime));
        }

        if (showNoclip) {
            rightText.push_back(fmt::format("Deaths: {}", Vars::noclipDeaths));
            rightText.push_back(fmt::format("Noclip: {:.2f}%", Vars::noclipAccuracy));
        }

        if (leftText.empty() && rightText.empty())
            return;

        bool hasCoins = false;
        if (m_playLayer && m_playLayer->m_level && m_playLayer->m_level->m_coins > 0)
            hasCoins = true;
        else if (bgl && bgl->m_level && bgl->m_level->m_coins > 0)
            hasCoins = true;
        else if (m_coinsToAnimate && m_coinsToAnimate->count() > 0)
            hasCoins = true;

        bool hasRewards = (m_stars > 0 || m_moons > 0 || m_orbs > 0 || m_diamonds > 0);

        std::string bottomMsg;
        if (!systemMsg.empty())
            bottomMsg = systemMsg;
        else if (Config::get().endscreenPhrases && !hasCoins)
            bottomMsg = nh::phrases::random();

        auto container = CCNode::create();
        container->setID("nh-endscreen-stats"_spr);

        std::vector<CCLabelBMFont*> leftLabels, rightLabels;
        const float leftWidth  = nhBuildColumn(leftText,  leftLabels);
        const float rightWidth = nhBuildColumn(rightText, rightLabels);

        const size_t rows = std::max(leftLabels.size(), rightLabels.size());

        if (rightLabels.empty()) {
            const float shiftX = hasRewards ? -48.f : 0.f;
            nhPlaceColumn(container, leftLabels, rows, shiftX);
        }
        else {
            const float gap     = hasRewards ? 36.f : kColGap;
            const float total   = leftWidth + gap + rightWidth;
            const float shiftX  = hasRewards ? -48.f : 0.f;
            const float boxLeft = -total / 2.f + shiftX;

            nhPlaceColumn(container, leftLabels, rows,
                          boxLeft + leftWidth / 2.f);
            nhPlaceColumn(container, rightLabels, rows,
                          boxLeft + leftWidth + gap + rightWidth / 2.f);
        }

        float blockSpan = (rows - 1) * kLineStep;

        if (!bottomMsg.empty()) {
            const float msgY = blockSpan + kMsgGap;

            auto msgLabel = CCLabelBMFont::create(bottomMsg.c_str(), "bigFont.fnt");
            msgLabel->setID("nh-endscreen-message"_spr);
            msgLabel->setAnchorPoint(ccp(0.5f, 0.5f));
            msgLabel->setScale(kMsgScale);

            float w = msgLabel->getScaledContentSize().width;
            if (w > kMaxWidth)
                msgLabel->setScale(kMsgScale * kMaxWidth / w);

            msgLabel->setPosition(ccp(0.f, -msgY));
            container->addChild(msgLabel);

            blockSpan = msgY;
        }

        float centerX, centerY;
        if (haveAnchor) {
            centerX = anchorX;
            centerY = (statTopY + statBotY) / 2.f;
        }
        else {
            CCSize box = m_mainLayer->getContentSize();
            if (box.width <= 0.f || box.height <= 0.f)
                box = CCDirector::get()->getWinSize();
            centerX = box.width / 2.f;
            centerY = box.height / 2.f;
        }

        container->setPosition(ccp(centerX, centerY + blockSpan / 2.f + kBlockOffset));

        m_mainLayer->addChild(container, 10);
    }
};
