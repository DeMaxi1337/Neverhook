#pragma once
#include "../Config.hpp"

namespace Vars {
    inline bool&  noclip         = Config::get().noclip;
    inline bool&  noDeathEffect  = Config::get().noDeathEffect;
    inline bool&  noRespawnFlash = Config::get().noRespawnFlash;
    inline bool&  practiceMusic  = Config::get().practiceMusic;
    inline bool&  iconBypass     = Config::get().iconBypass;
    inline bool&  speedhack      = Config::get().speedhack;
    inline float& speedhackValue = Config::get().speedhackValue;
    inline bool&  fpsUnlock      = Config::get().fpsUnlock;
    inline float& fpsValue       = Config::get().fpsValue;
    inline bool&  verifyHack     = Config::get().verifyHack;
    inline bool&  copyHack       = Config::get().copyHack;

    inline bool&  hideAttempts          = Config::get().hideAttempts;
    inline bool&  accuratePercent       = Config::get().accuratePercent;
    inline int&   accuratePercentDigits = Config::get().accuratePercentDigits;
    inline bool&  noGlow                = Config::get().noGlow;
    inline bool&  noCameraShake         = Config::get().noCameraShake;
    inline bool&  showHitboxes          = Config::get().showHitboxes;
    inline bool&  showHitboxesOnDeath   = Config::get().showHitboxesOnDeath;

    inline bool&  tpsBypass             = Config::get().tpsBypass;
    inline float& tpsValue              = Config::get().tpsValue;

    inline bool&  noMirrorPortal        = Config::get().noMirrorPortal;
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

    inline bool&  noTransition   = Config::get().noTransition;
    inline bool&  noPauseButton  = Config::get().noPauseButton;
    inline bool&  speedhackAudio = Config::get().speedhackAudio;

    // is the menu currently open?
    inline bool   menuOpen       = false;
}
