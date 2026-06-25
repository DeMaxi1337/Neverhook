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

    // is the menu currently open?
    inline bool   menuOpen       = false;
}
