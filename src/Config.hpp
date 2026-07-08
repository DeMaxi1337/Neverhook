#pragma once
#include <Geode/Geode.hpp>

// GD advances m_gameState.m_currentProgress by two units per physics tick,
// whereas the conventional frame number (as shown by Mega Hack and TAS tools)
// counts one per tick. Divide the raw progress by this to get that frame
// number. Single source of truth so the conversion never drifts across files.
inline constexpr int kProgressPerFrame = 2;

struct Config {
    bool  noclip          = false;
    bool   noclipTint        = false;
    float  noclipTintColor[4] = { 1.00f, 0.10f, 0.10f, 1.00f };
    float  noclipTintOpacity = 90.f;
    float  noclipTintTime    = 0.50f;
    bool   noclipHitsound    = false;
    float  noclipHitsoundVolume = 100.f;
    int    noclipDeaths      = 0;
    double noclipAccuracy    = 100.0;
    bool  noDeathEffect   = false;
    bool  noRespawnFlash  = false;
    bool  practiceMusic   = false;
    bool  practiceFix      = false;
    bool  iconBypass      = false;
    bool  speedhack       = false;
    float speedhackValue  = 1.0f;
    bool  fpsUnlock       = false;
    float fpsValue        = 240.0f;
    bool  verifyHack      = false;
    bool  copyHack        = false;

    bool  hideAttempts          = false;
    bool  accuratePercent       = false;
    int   accuratePercentDigits = 2;
    bool  noGlow                = false;
    bool  noCameraShake         = false;
    bool  noEndShake            = false;
    bool  showHitboxes          = false;
    bool  showHitboxesOnDeath   = false;
    bool  showTrajectory        = false;

    bool  tpsBypass             = false;
    float tpsValue              = 240.0f;
    // bool  compactLists          = false;

    bool  noMirrorPortal        = false;
    bool  smartStartpos         = true;
    bool  startposSwitcher      = false;
    bool  frameAdvance          = false;
    int   faStepKey             = 67;
    bool  faHold                = false;
    float faHoldDelayCfg        = 0.25f;
    int   faHoldSpeedCfg        = 5;
    bool  instantComplete       = false;
    bool  instantRestart        = false;
    bool  customRespawn         = false;
    float respawnTime           = 1.0f;
    bool  safeMode              = false;
    bool  safeFreezeAttempts    = true;
    bool  safeFreezeJumps       = true;

    bool  noDashFire        = false;
    bool  noSpiderDash      = false;
    bool  noWavePulse       = false;
    bool  noWaveTrail       = false;
    bool  solidWaveTrail    = false;
    bool  waveTrailSize     = false;
    float waveTrailSizeValue = 1.0f;
    bool  noParticles       = false;
    bool  noTrail           = false;

    bool  hideEditorUI       = false;
    bool  levelEdit          = false;
    bool  noCustomObjLimit   = false;
    bool  noZoomLimit        = false;
    bool  toolboxButtonBypass = false;
    bool  sliderLimitBypass  = false;

    bool  jumpHack          = false;
    bool  allModesPlatformer = false;
    bool  hitboxMultiplier  = false;
    float hitboxMultPlayer  = 1.0f;
    float hitboxMultSolid   = 1.0f;
    float hitboxMultHazard  = 1.0f;

    bool  autoclicker      = false;
    bool  autoclickerP2    = false;
    float autoclickerCps   = 10.0f;
    float autoclickerP2Cps = 10.0f;

    bool  noTransition   = false;
    bool  noPauseButton  = false;
    bool  speedhackAudio = false;

    bool  watermark        = false;
    int   watermarkStyle   = 0;
    int   watermarkLine    = 1;
    int   watermarkPos     = 2;
    bool  wmGlow           = true;
    bool  wmGlobal         = true;
    bool  wmBackground     = true;
    bool  wmShowName       = true;
    bool  wmShowUser       = true;
    bool  wmShowFps        = true;
    bool  wmShowTime       = true;
    bool  wmShowNcAcc      = false;
    bool  wmShowNcDeaths   = false;
    bool  wmShowFrame      = false;
    int   wmNameColorMode  = 0;
    float wmColorOne[4]    = { 0.30f, 0.49f, 1.00f, 1.00f };
    float wmColorTwo[4]    = { 0.65f, 0.80f, 1.00f, 1.00f };

    bool  keybindsList     = false;
    int   keybindsStyle    = 0;
    int   keybindsLine     = 1;
    int   keybindsPos      = 0;
    bool  kbGlow           = true;
    bool  kbGlobal         = true;
    bool  kbBackground     = true;
    float keybindsX        = 0.012f;
    float keybindsY        = 0.045f;

    // --- Ported level / bypass functions ---
    bool  noShader = false;
    bool  noPortalLightning = false;
    bool  hideLevelCompleteVfx = false;
    bool  noMusicFadeOut = false;
    bool  autoPracticeMode = false;
    bool  autoPickupCoins = false;
    bool  pauseDuringComplete = false;
    bool  autoSongDownload = false;
    bool  layoutMode = false;
    bool  unlockMainLevels = false;
    bool  unlockShops = false;
    bool  unlockVaults = false;

    static Config& get() {
        static Config inst;
        return inst;
    }

    void load();
    void save();
};

void applyFPS();
