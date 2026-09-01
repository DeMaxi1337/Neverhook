#pragma once
#include "../Config.hpp"

namespace Vars {
    inline bool&  noclip         = Config::get().noclip;
    inline bool&   noclipTint       = Config::get().noclipTint;
    inline float*  noclipTintColor  = Config::get().noclipTintColor;
    inline float&  noclipTintOpacity= Config::get().noclipTintOpacity;
    inline float&  noclipTintTime   = Config::get().noclipTintTime;
    inline bool&   noclipHitsound   = Config::get().noclipHitsound;
    inline float&  noclipHitsoundVolume = Config::get().noclipHitsoundVolume;
    inline int&    noclipDeaths     = Config::get().noclipDeaths;
    inline double& noclipAccuracy   = Config::get().noclipAccuracy;
    inline bool&  noDeathEffect  = Config::get().noDeathEffect;
    inline bool&  noRespawnFlash = Config::get().noRespawnFlash;
    inline bool&  practiceMusic  = Config::get().practiceMusic;
    inline bool&  practiceFix     = Config::get().practiceFix;
    inline bool&  iconBypass     = Config::get().iconBypass;
    inline bool&  speedhack      = Config::get().speedhack;
    inline float& speedhackValue = Config::get().speedhackValue;
    inline bool&  fpsUnlock      = Config::get().fpsUnlock;
    inline float& fpsValue       = Config::get().fpsValue;
    inline bool&  frameExtrapolation = Config::get().frameExtrapolation;
    inline bool&  verifyHack     = Config::get().verifyHack;
    inline bool&  copyHack       = Config::get().copyHack;

    inline bool&  hideAttempts          = Config::get().hideAttempts;
    inline bool&  accuratePercent       = Config::get().accuratePercent;
    inline int&   accuratePercentDigits = Config::get().accuratePercentDigits;
    inline bool&  noGlow                = Config::get().noGlow;
    inline bool&  noCameraShake         = Config::get().noCameraShake;
    inline bool&  noEndShake            = Config::get().noEndShake;
    inline bool&  noDeathShake          = Config::get().noDeathShake;
    inline bool&  showHitboxes          = Config::get().showHitboxes;
    inline bool&  showHitboxesOnDeath   = Config::get().showHitboxesOnDeath;
    inline bool&  showTrajectory        = Config::get().showTrajectory;
    inline bool&  clickBetweenFrames    = Config::get().clickBetweenFrames;

    inline bool&  tpsBypass             = Config::get().tpsBypass;
    inline float& tpsValue              = Config::get().tpsValue;
    inline bool&  compactList           = Config::get().compactList;

    inline bool&  noMirrorPortal        = Config::get().noMirrorPortal;
    inline bool&  smartStartpos         = Config::get().smartStartpos;
    inline bool&  startposSwitcher      = Config::get().startposSwitcher;
    inline bool&  frameAdvance          = Config::get().frameAdvance;
    inline int&   faStepKey             = Config::get().faStepKey;
    inline bool&  faHold                = Config::get().faHold;
    inline float& faHoldDelayCfg        = Config::get().faHoldDelayCfg;
    inline int&   faHoldSpeedCfg        = Config::get().faHoldSpeedCfg;
    inline bool&  instantComplete       = Config::get().instantComplete;
    inline bool&  instantRestart        = Config::get().instantRestart;
    inline bool&  customRespawn         = Config::get().customRespawn;
    inline float& respawnTime           = Config::get().respawnTime;
    inline bool&  safeMode              = Config::get().safeMode;
    inline bool&  safeFreezeAttempts    = Config::get().safeFreezeAttempts;
    inline bool&  safeFreezeJumps       = Config::get().safeFreezeJumps;

    inline bool&  noDashFire        = Config::get().noDashFire;
    inline bool&  noSpiderDash      = Config::get().noSpiderDash;
    inline bool&  noWavePulse       = Config::get().noWavePulse;
    inline bool&  noWaveTrail       = Config::get().noWaveTrail;
    inline bool&  solidWaveTrail    = Config::get().solidWaveTrail;
    inline bool&  waveTrailSize     = Config::get().waveTrailSize;
    inline float& waveTrailSizeValue = Config::get().waveTrailSizeValue;
    inline bool&  noParticles       = Config::get().noParticles;
    inline bool&  noTrail           = Config::get().noTrail;

    inline bool&  hideEditorUI       = Config::get().hideEditorUI;
    inline bool&  levelEdit          = Config::get().levelEdit;
    inline bool&  noCustomObjLimit   = Config::get().noCustomObjLimit;
    inline bool&  noZoomLimit        = Config::get().noZoomLimit;
    inline bool&  toolboxButtonBypass = Config::get().toolboxButtonBypass;
    inline bool&  sliderLimitBypass  = Config::get().sliderLimitBypass;

    inline bool&  jumpHack           = Config::get().jumpHack;
    inline bool&  allModesPlatformer = Config::get().allModesPlatformer;
    inline bool&  hitboxMultiplier   = Config::get().hitboxMultiplier;
    inline float& hitboxMultPlayer   = Config::get().hitboxMultPlayer;
    inline float& hitboxMultSolid    = Config::get().hitboxMultSolid;
    inline float& hitboxMultHazard   = Config::get().hitboxMultHazard;

    inline bool&  autoclicker      = Config::get().autoclicker;
    inline bool&  autoclickerP2    = Config::get().autoclickerP2;
    inline float& autoclickerCps   = Config::get().autoclickerCps;
    inline float& autoclickerP2Cps = Config::get().autoclickerP2Cps;

    inline int&   macroPlaybackAttempt   = Config::get().macroPlaybackAttempt;
    inline bool&  macroIgnoreInputs      = Config::get().macroIgnoreInputs;
    inline bool&  macroAutoPlayback      = Config::get().macroAutoPlayback;

    inline bool&  noTransition   = Config::get().noTransition;
    inline bool&  noPauseButton  = Config::get().noPauseButton;
    inline bool&  speedhackAudio = Config::get().speedhackAudio;

    inline bool&  watermark       = Config::get().watermark;
    inline int&   watermarkStyle  = Config::get().watermarkStyle;
    inline int&   watermarkLine   = Config::get().watermarkLine;
    inline int&   watermarkPos    = Config::get().watermarkPos;
    inline bool&  wmGlow          = Config::get().wmGlow;
    inline bool&  wmGlobal        = Config::get().wmGlobal;
    inline bool&  wmBackground    = Config::get().wmBackground;
    inline bool&  wmShowName      = Config::get().wmShowName;
    inline bool&  wmShowUser      = Config::get().wmShowUser;
    inline bool&  wmShowFps       = Config::get().wmShowFps;
    inline bool&  wmShowTime      = Config::get().wmShowTime;
    inline bool&   wmShowNcAcc      = Config::get().wmShowNcAcc;
    inline bool&   wmShowNcDeaths   = Config::get().wmShowNcDeaths;
    inline bool&   wmShowFrame      = Config::get().wmShowFrame;
    inline int&   wmNameColorMode = Config::get().wmNameColorMode;
    inline float* wmColorOne      = Config::get().wmColorOne;
    inline float* wmColorTwo      = Config::get().wmColorTwo;

    inline bool&  keybindsList    = Config::get().keybindsList;
    inline int&   keybindsStyle   = Config::get().keybindsStyle;
    inline int&   keybindsLine    = Config::get().keybindsLine;
    inline int&   keybindsPos     = Config::get().keybindsPos;
    inline bool&  kbGlow          = Config::get().kbGlow;
    inline bool&  kbGlobal        = Config::get().kbGlobal;
    inline bool&  kbBackground    = Config::get().kbBackground;
    inline float& keybindsX       = Config::get().keybindsX;
    inline float& keybindsY       = Config::get().keybindsY;

    inline bool&  noShader = Config::get().noShader;
    inline bool&  noPortalLightning = Config::get().noPortalLightning;
    inline bool&  hideLevelCompleteVfx = Config::get().hideLevelCompleteVfx;
    inline bool&  noMusicFadeOut = Config::get().noMusicFadeOut;
    inline bool&  autoPracticeMode = Config::get().autoPracticeMode;
    inline bool&  autoPickupCoins = Config::get().autoPickupCoins;
    inline bool&  pauseDuringComplete = Config::get().pauseDuringComplete;
    inline bool&  hidePauseMenu = Config::get().hidePauseMenu;
    inline bool&  mouseZoomOnPause = Config::get().mouseZoomOnPause;
    inline bool&  autoSongDownload = Config::get().autoSongDownload;
    inline bool&  layoutMode = Config::get().layoutMode;
    inline bool&  unlockMainLevels = Config::get().unlockMainLevels;
    inline bool&  unlockShops = Config::get().unlockShops;
    inline bool&  unlockVaults = Config::get().unlockVaults;

    inline bool&  hidePlayer        = Config::get().hidePlayer;
    inline bool&  playerOnTop       = Config::get().playerOnTop;
    inline bool&  noRobotFire       = Config::get().noRobotFire;
    inline bool&  noSwingFire       = Config::get().noSwingFire;
    inline bool&  noGhostTrail      = Config::get().noGhostTrail;
    inline bool&  noTrailBehindWave = Config::get().noTrailBehindWave;
    inline bool&  noCircleWave      = Config::get().noCircleWave;
    inline bool&  randomSeed        = Config::get().randomSeed;
    inline int&   randomSeedValue   = Config::get().randomSeedValue;

    inline bool&  autoSave      = Config::get().autoSave;
    inline bool&  guiBlur       = Config::get().guiBlur;
    inline float& guiBlurStrength = Config::get().guiBlurStrength;
    inline int&   menuScale     = Config::get().menuScale;
    inline float& menuAnimSpeed = Config::get().menuAnimSpeed;

    inline bool&  endscreenStats   = Config::get().endscreenStats;
    inline bool&  endscreenPhrases = Config::get().endscreenPhrases;

    inline bool&  hideStatus                 = Config::get().hideStatus;
    inline float& statusOpacity              = Config::get().statusOpacity;
    inline float& statusScale                = Config::get().statusScale;
    inline int&   statusFont                 = Config::get().statusFont;
    inline bool&  statusBg                   = Config::get().statusBg;

    inline int&   statusCheatIndicator       = Config::get().statusCheatIndicator;
    inline int&   statusCheatIndicatorMode   = Config::get().statusCheatIndicatorMode;
    inline int&   statusFps                  = Config::get().statusFps;
    inline int&   statusCps                  = Config::get().statusCps;
    inline int&   statusBestRun              = Config::get().statusBestRun;
    inline int&   statusNoclipAcc            = Config::get().statusNoclipAcc;
    inline int&   statusNoclipDeaths         = Config::get().statusNoclipDeaths;
    inline int&   statusAttempts             = Config::get().statusAttempts;
    inline int&   statusJumps                = Config::get().statusJumps;
    inline int&   statusPercentage           = Config::get().statusPercentage;
    inline int&   statusLevelTime            = Config::get().statusLevelTime;
    inline int&   statusSessionTime          = Config::get().statusSessionTime;
    inline int&   statusClock                = Config::get().statusClock;
    inline int&   statusFrameCounter         = Config::get().statusFrameCounter;
    inline int&   statusPosition             = Config::get().statusPosition;
    inline int&   statusVelocity             = Config::get().statusVelocity;
    inline int&   statusMessage              = Config::get().statusMessage;
    inline std::string& statusMessageText    = Config::get().statusMessageText;
    inline int&   statusTestmode             = Config::get().statusTestmode;
    inline int&   statusReplayState          = Config::get().statusReplayState;
    inline std::vector<int>& statusOrder     = Config::get().statusOrder;

    inline bool   menuOpen       = false;
}
