#include <Geode/Geode.hpp>
#include <Geode/modify/CCDirector.hpp>
#include "Config.hpp"
#include "gui/binds.h"

using namespace geode::prelude;

#define NH_LOAD_VAL(field, def) field = m->getSavedValue<decltype(field)>(#field, def)
#define NH_LOAD_FLT(field, def) field = static_cast<float>(m->getSavedValue<double>(#field, def))
#define NH_SAVE_VAL(field) m->setSavedValue(#field, field)
#define NH_SAVE_FLT(field) m->setSavedValue(#field, static_cast<double>(field))

#define NH_LOAD_COL4(field, r, g, b, a) \
    field[0] = static_cast<float>(m->getSavedValue<double>(#field "0", r)); \
    field[1] = static_cast<float>(m->getSavedValue<double>(#field "1", g)); \
    field[2] = static_cast<float>(m->getSavedValue<double>(#field "2", b)); \
    field[3] = static_cast<float>(m->getSavedValue<double>(#field "3", a))

#define NH_SAVE_COL4(field) \
    m->setSavedValue(#field "0", static_cast<double>(field[0])); \
    m->setSavedValue(#field "1", static_cast<double>(field[1])); \
    m->setSavedValue(#field "2", static_cast<double>(field[2])); \
    m->setSavedValue(#field "3", static_cast<double>(field[3]))

void Config::load() {
    auto m = Mod::get();
    NH_LOAD_VAL(noclip, false);
    NH_LOAD_VAL(noclipTint, false);
    NH_LOAD_COL4(noclipTintColor, 1.00, 0.10, 0.10, 1.00);
    NH_LOAD_FLT(noclipTintOpacity, 90.0);
    NH_LOAD_FLT(noclipTintTime, 0.50);
    NH_LOAD_VAL(noclipHitsound, false);
    NH_LOAD_FLT(noclipHitsoundVolume, 100.0);
    NH_LOAD_VAL(noDeathEffect, false);
    NH_LOAD_VAL(noRespawnFlash, false);
    NH_LOAD_VAL(practiceMusic, false);
    NH_LOAD_VAL(practiceFix, false);
    NH_LOAD_VAL(iconBypass, false);
    NH_LOAD_VAL(speedhack, false);
    NH_LOAD_FLT(speedhackValue, 1.0);
    NH_LOAD_VAL(fpsUnlock, false);
    NH_LOAD_FLT(fpsValue, 240.0);
    NH_LOAD_VAL(frameExtrapolation, false);
    NH_LOAD_VAL(verifyHack, false);
    NH_LOAD_VAL(copyHack, false);
    NH_LOAD_VAL(hideAttempts, false);
    NH_LOAD_VAL(accuratePercent, false);
    NH_LOAD_VAL(accuratePercentDigits, 2);
    NH_LOAD_VAL(noGlow, false);
    NH_LOAD_VAL(noCameraShake, false);
    NH_LOAD_VAL(noEndShake, false);
    NH_LOAD_VAL(noDeathShake, false);
    NH_LOAD_VAL(showHitboxes, false);
    NH_LOAD_VAL(showHitboxesOnDeath, false);
    NH_LOAD_VAL(showTrajectory, false);
    NH_LOAD_VAL(clickBetweenFrames, false);
    NH_LOAD_VAL(noShader, false);
    NH_LOAD_VAL(noPortalLightning, false);
    NH_LOAD_VAL(hideLevelCompleteVfx, false);
    NH_LOAD_VAL(noMusicFadeOut, false);
    NH_LOAD_VAL(autoPracticeMode, false);
    NH_LOAD_VAL(autoPickupCoins, false);
    NH_LOAD_VAL(pauseDuringComplete, false);
    NH_LOAD_VAL(hidePauseMenu, false);
    NH_LOAD_VAL(mouseZoomOnPause, false);
    NH_LOAD_VAL(autoSongDownload, false);
    NH_LOAD_VAL(layoutMode, false);
    NH_LOAD_VAL(unlockMainLevels, false);
    NH_LOAD_VAL(unlockShops, false);
    NH_LOAD_VAL(unlockVaults, false);
    NH_LOAD_VAL(tpsBypass, false);
    NH_LOAD_FLT(tpsValue, 240.0);
    NH_LOAD_VAL(compactList, false);
    NH_LOAD_VAL(noMirrorPortal, false);
    NH_LOAD_VAL(smartStartpos, true);
    NH_LOAD_VAL(startposSwitcher, false);
    NH_LOAD_VAL(frameAdvance, false);
    NH_LOAD_VAL(faStepKey, 67);
    NH_LOAD_VAL(faHold, false);
    NH_LOAD_FLT(faHoldDelayCfg, 0.25);
    NH_LOAD_VAL(faHoldSpeedCfg, 5);
    NH_LOAD_VAL(instantComplete, false);
    NH_LOAD_VAL(instantRestart, false);
    NH_LOAD_VAL(customRespawn, false);
    NH_LOAD_FLT(respawnTime, 1.0);
    NH_LOAD_VAL(safeMode, false);
    NH_LOAD_VAL(safeFreezeAttempts, true);
    NH_LOAD_VAL(safeFreezeJumps, true);

    NH_LOAD_VAL(noDashFire, false);
    NH_LOAD_VAL(noSpiderDash, false);
    NH_LOAD_VAL(noWavePulse, false);
    NH_LOAD_VAL(noWaveTrail, false);
    NH_LOAD_VAL(solidWaveTrail, false);
    NH_LOAD_VAL(waveTrailSize, false);
    NH_LOAD_FLT(waveTrailSizeValue, 1.0);
    NH_LOAD_VAL(noParticles, false);
    NH_LOAD_VAL(noTrail, false);
    NH_LOAD_VAL(hidePlayer, false);
    NH_LOAD_VAL(playerOnTop, false);
    NH_LOAD_VAL(noRobotFire, false);
    NH_LOAD_VAL(noSwingFire, false);
    NH_LOAD_VAL(noGhostTrail, false);
    NH_LOAD_VAL(noTrailBehindWave, false);
    NH_LOAD_VAL(noCircleWave, false);
    NH_LOAD_VAL(randomSeed, false);
    NH_LOAD_VAL(randomSeedValue, 0);

    NH_LOAD_VAL(hideEditorUI, false);
    NH_LOAD_VAL(levelEdit, false);
    NH_LOAD_VAL(noCustomObjLimit, false);
    NH_LOAD_VAL(noZoomLimit, false);
    NH_LOAD_VAL(toolboxButtonBypass, false);
    NH_LOAD_VAL(sliderLimitBypass, false);

    NH_LOAD_VAL(jumpHack, false);
    NH_LOAD_VAL(allModesPlatformer, false);
    NH_LOAD_VAL(hitboxMultiplier, false);
    NH_LOAD_FLT(hitboxMultPlayer, 1.0);
    NH_LOAD_FLT(hitboxMultSolid, 1.0);
    NH_LOAD_FLT(hitboxMultHazard, 1.0);

    NH_LOAD_VAL(autoclicker, false);
    NH_LOAD_VAL(autoclickerP2, false);
    NH_LOAD_FLT(autoclickerCps, 10.0);
    NH_LOAD_FLT(autoclickerP2Cps, 10.0);
    NH_LOAD_VAL(macroPlaybackAttempt, 0);
    NH_LOAD_VAL(macroIgnoreInputs, false);
    NH_LOAD_VAL(macroAutoPlayback, false);

    NH_LOAD_VAL(noTransition, false);
    NH_LOAD_VAL(noPauseButton, false);
    NH_LOAD_VAL(speedhackAudio, false);

    NH_LOAD_VAL(watermark, false);
    NH_LOAD_VAL(watermarkStyle, 0);
    NH_LOAD_VAL(watermarkLine, 1);
    NH_LOAD_VAL(watermarkPos, 2);
    NH_LOAD_VAL(wmGlow, true);
    NH_LOAD_VAL(wmGlobal, true);
    NH_LOAD_VAL(wmBackground, true);
    NH_LOAD_VAL(wmShowName, true);
    NH_LOAD_VAL(wmShowUser, true);
    NH_LOAD_VAL(wmShowFps, true);
    NH_LOAD_VAL(wmShowTime, true);
    NH_LOAD_VAL(wmShowNcAcc, false);
    NH_LOAD_VAL(wmShowNcDeaths, false);
    NH_LOAD_VAL(wmShowFrame, false);
    NH_LOAD_VAL(wmNameColorMode, 0);
    NH_LOAD_COL4(wmColorOne, 0.30, 0.49, 1.00, 1.00);
    NH_LOAD_COL4(wmColorTwo, 0.65, 0.80, 1.00, 1.00);

    NH_LOAD_VAL(keybindsList, false);
    NH_LOAD_VAL(keybindsStyle, 0);
    NH_LOAD_VAL(keybindsLine, 1);
    NH_LOAD_VAL(keybindsPos, 0);
    NH_LOAD_VAL(kbGlow, true);
    NH_LOAD_VAL(kbGlobal, true);
    NH_LOAD_VAL(kbBackground, true);
    NH_LOAD_FLT(keybindsX, 0.012);
    NH_LOAD_FLT(keybindsY, 0.045);

    NH_LOAD_VAL(autoSave, false);
    NH_LOAD_VAL(guiBlur, true);
    NH_LOAD_FLT(guiBlurStrength, 3.0);
    NH_LOAD_VAL(menuScale, 0);
    NH_LOAD_FLT(menuAnimSpeed, 1.0);

    NH_LOAD_VAL(hideStatus, false);
    NH_LOAD_FLT(statusOpacity, 70.0);
    NH_LOAD_FLT(statusScale, 0.5);
    NH_LOAD_VAL(statusFont, 0);
    NH_LOAD_VAL(statusBg, false);

    NH_LOAD_VAL(statusCheatIndicator, 0);
    NH_LOAD_VAL(statusCheatIndicatorMode, 0);
    NH_LOAD_VAL(statusFps, 0);
    NH_LOAD_VAL(statusCps, 0);
    NH_LOAD_VAL(statusBestRun, 0);
    NH_LOAD_VAL(statusNoclipAcc, 0);
    NH_LOAD_VAL(statusNoclipDeaths, 0);
    NH_LOAD_VAL(statusAttempts, 0);
    NH_LOAD_VAL(statusJumps, 0);
    NH_LOAD_VAL(statusPercentage, 0);
    NH_LOAD_VAL(statusLevelTime, 0);
    NH_LOAD_VAL(statusSessionTime, 0);
    NH_LOAD_VAL(statusClock, 0);
    NH_LOAD_VAL(statusFrameCounter, 0);
    NH_LOAD_VAL(statusMessage, 0);
    statusMessageText = m->getSavedValue<std::string>("cfg_statusMessageText", "#ApproveNeverhook");
    NH_LOAD_VAL(statusTestmode, 0);
    NH_LOAD_VAL(statusReplayState, 0);
    statusOrder = m->getSavedValue<std::vector<int>>("cfg_statusOrder", { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17 });
}

void Config::save() {
    auto m = Mod::get();
    NH_SAVE_VAL(noclip);
    NH_SAVE_VAL(noclipTint);
    NH_SAVE_COL4(noclipTintColor);
    NH_SAVE_FLT(noclipTintOpacity);
    NH_SAVE_FLT(noclipTintTime);
    NH_SAVE_VAL(noclipHitsound);
    NH_SAVE_FLT(noclipHitsoundVolume);
    NH_SAVE_VAL(noDeathEffect);
    NH_SAVE_VAL(noRespawnFlash);
    NH_SAVE_VAL(practiceMusic);
    NH_SAVE_VAL(practiceFix);
    NH_SAVE_VAL(iconBypass);
    NH_SAVE_VAL(speedhack);
    NH_SAVE_FLT(speedhackValue);
    NH_SAVE_VAL(fpsUnlock);
    NH_SAVE_FLT(fpsValue);
    NH_SAVE_VAL(frameExtrapolation);
    NH_SAVE_VAL(verifyHack);
    NH_SAVE_VAL(copyHack);
    NH_SAVE_VAL(hideAttempts);
    NH_SAVE_VAL(accuratePercent);
    NH_SAVE_VAL(accuratePercentDigits);
    NH_SAVE_VAL(noGlow);
    NH_SAVE_VAL(noCameraShake);
    NH_SAVE_VAL(noEndShake);
    NH_SAVE_VAL(noDeathShake);
    NH_SAVE_VAL(showHitboxes);
    NH_SAVE_VAL(showHitboxesOnDeath);
    NH_SAVE_VAL(showTrajectory);
    NH_SAVE_VAL(clickBetweenFrames);
    NH_SAVE_VAL(noShader);
    NH_SAVE_VAL(noPortalLightning);
    NH_SAVE_VAL(hideLevelCompleteVfx);
    NH_SAVE_VAL(noMusicFadeOut);
    NH_SAVE_VAL(autoPracticeMode);
    NH_SAVE_VAL(autoPickupCoins);
    NH_SAVE_VAL(pauseDuringComplete);
    NH_SAVE_VAL(hidePauseMenu);
    NH_SAVE_VAL(mouseZoomOnPause);
    NH_SAVE_VAL(autoSongDownload);
    NH_SAVE_VAL(layoutMode);
    NH_SAVE_VAL(unlockMainLevels);
    NH_SAVE_VAL(unlockShops);
    NH_SAVE_VAL(unlockVaults);
    NH_SAVE_VAL(tpsBypass);
    NH_SAVE_FLT(tpsValue);
    NH_SAVE_VAL(compactList);
    NH_SAVE_VAL(noMirrorPortal);
    NH_SAVE_VAL(smartStartpos);
    NH_SAVE_VAL(startposSwitcher);
    NH_SAVE_VAL(frameAdvance);
    NH_SAVE_VAL(faStepKey);
    NH_SAVE_VAL(faHold);
    NH_SAVE_FLT(faHoldDelayCfg);
    NH_SAVE_VAL(faHoldSpeedCfg);
    NH_SAVE_VAL(instantComplete);
    NH_SAVE_VAL(instantRestart);
    NH_SAVE_VAL(customRespawn);
    NH_SAVE_FLT(respawnTime);
    NH_SAVE_VAL(safeMode);
    NH_SAVE_VAL(safeFreezeAttempts);
    NH_SAVE_VAL(safeFreezeJumps);

    NH_SAVE_VAL(noDashFire);
    NH_SAVE_VAL(noSpiderDash);
    NH_SAVE_VAL(noWavePulse);
    NH_SAVE_VAL(noWaveTrail);
    NH_SAVE_VAL(solidWaveTrail);
    NH_SAVE_VAL(waveTrailSize);
    NH_SAVE_FLT(waveTrailSizeValue);
    NH_SAVE_VAL(noParticles);
    NH_SAVE_VAL(noTrail);
    NH_SAVE_VAL(hidePlayer);
    NH_SAVE_VAL(playerOnTop);
    NH_SAVE_VAL(noRobotFire);
    NH_SAVE_VAL(noSwingFire);
    NH_SAVE_VAL(noGhostTrail);
    NH_SAVE_VAL(noTrailBehindWave);
    NH_SAVE_VAL(noCircleWave);
    NH_SAVE_VAL(randomSeed);
    NH_SAVE_VAL(randomSeedValue);

    NH_SAVE_VAL(hideEditorUI);
    NH_SAVE_VAL(levelEdit);
    NH_SAVE_VAL(noCustomObjLimit);
    NH_SAVE_VAL(noZoomLimit);
    NH_SAVE_VAL(toolboxButtonBypass);
    NH_SAVE_VAL(sliderLimitBypass);

    NH_SAVE_VAL(jumpHack);
    NH_SAVE_VAL(allModesPlatformer);
    NH_SAVE_VAL(hitboxMultiplier);
    NH_SAVE_FLT(hitboxMultPlayer);
    NH_SAVE_FLT(hitboxMultSolid);
    NH_SAVE_FLT(hitboxMultHazard);

    NH_SAVE_VAL(autoclicker);
    NH_SAVE_VAL(autoclickerP2);
    NH_SAVE_FLT(autoclickerCps);
    NH_SAVE_FLT(autoclickerP2Cps);
    NH_SAVE_VAL(macroPlaybackAttempt);
    NH_SAVE_VAL(macroIgnoreInputs);
    NH_SAVE_VAL(macroAutoPlayback);

    NH_SAVE_VAL(noTransition);
    NH_SAVE_VAL(noPauseButton);
    NH_SAVE_VAL(speedhackAudio);

    NH_SAVE_VAL(watermark);
    NH_SAVE_VAL(watermarkStyle);
    NH_SAVE_VAL(watermarkLine);
    NH_SAVE_VAL(watermarkPos);
    NH_SAVE_VAL(wmGlow);
    NH_SAVE_VAL(wmGlobal);
    NH_SAVE_VAL(wmBackground);
    NH_SAVE_VAL(wmShowName);
    NH_SAVE_VAL(wmShowUser);
    NH_SAVE_VAL(wmShowFps);
    NH_SAVE_VAL(wmShowTime);
    NH_SAVE_VAL(wmShowNcAcc);
    NH_SAVE_VAL(wmShowNcDeaths);
    NH_SAVE_VAL(wmShowFrame);
    NH_SAVE_VAL(wmNameColorMode);
    NH_SAVE_COL4(wmColorOne);
    NH_SAVE_COL4(wmColorTwo);

    NH_SAVE_VAL(keybindsList);
    NH_SAVE_VAL(keybindsStyle);
    NH_SAVE_VAL(keybindsLine);
    NH_SAVE_VAL(keybindsPos);
    NH_SAVE_VAL(kbGlow);
    NH_SAVE_VAL(kbGlobal);
    NH_SAVE_VAL(kbBackground);
    NH_SAVE_FLT(keybindsX);
    NH_SAVE_FLT(keybindsY);

    NH_SAVE_VAL(autoSave);
    NH_SAVE_VAL(guiBlur);
    NH_SAVE_FLT(guiBlurStrength);
    NH_SAVE_VAL(menuScale);
    NH_SAVE_FLT(menuAnimSpeed);

    NH_SAVE_VAL(hideStatus);
    NH_SAVE_FLT(statusOpacity);
    NH_SAVE_FLT(statusScale);
    NH_SAVE_VAL(statusFont);
    NH_SAVE_VAL(statusBg);

    NH_SAVE_VAL(statusCheatIndicator);
    NH_SAVE_VAL(statusCheatIndicatorMode);
    NH_SAVE_VAL(statusFps);
    NH_SAVE_VAL(statusCps);
    NH_SAVE_VAL(statusBestRun);
    NH_SAVE_VAL(statusNoclipAcc);
    NH_SAVE_VAL(statusNoclipDeaths);
    NH_SAVE_VAL(statusAttempts);
    NH_SAVE_VAL(statusJumps);
    NH_SAVE_VAL(statusPercentage);
    NH_SAVE_VAL(statusLevelTime);
    NH_SAVE_VAL(statusSessionTime);
    NH_SAVE_VAL(statusClock);
    NH_SAVE_VAL(statusFrameCounter);
    NH_SAVE_VAL(statusPosition);
    NH_SAVE_VAL(statusVelocity);
    NH_SAVE_VAL(statusMessage);
    m->setSavedValue("cfg_statusMessageText", statusMessageText);
    NH_SAVE_VAL(statusTestmode);
    NH_SAVE_VAL(statusReplayState);
    m->setSavedValue("cfg_statusOrder", statusOrder);
}

void applyFPS() {
    auto& c = Config::get();
    double fps = (double)c.fpsValue;
    if (fps < 1.0) fps = 1.0;
    double interval = c.fpsUnlock ? (1.0 / fps) : (1.0 / 60.0);
    if (auto app = CCApplication::sharedApplication()) {
        app->setAnimationInterval(interval);
    }
}

$on_mod(Loaded) {
    Config::get().load();
    BindSystem::get().registerAll();
    BindSystem::get().load(Mod::get());
    applyFPS();
    log::info("Neverhook loaded - press INSERT in game to open the menu");
}

class $modify(NHDirectorSave, CCDirector) {
    void willSwitchToScene(CCScene* scene) {
        CCDirector::willSwitchToScene(scene);
        Config::get().save();
        BindSystem::get().save(Mod::get());
    }
};
