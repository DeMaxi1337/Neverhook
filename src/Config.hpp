#pragma once
#include <Geode/Geode.hpp>

struct Config {
    bool  noclip          = false;
    bool  noDeathEffect   = false;
    bool  noRespawnFlash  = false;
    bool  practiceMusic   = false;
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
    bool  showHitboxes          = false;
    bool  showHitboxesOnDeath   = false;

    bool  tpsBypass             = false;
    float tpsValue              = 240.0f;

    bool  noMirrorPortal        = false;
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

    static Config& get() {
        static Config inst;
        return inst;
    }

    void load();
    void save();
};

void applyFPS();
