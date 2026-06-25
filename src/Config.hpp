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

    static Config& get() {
        static Config inst;
        return inst;
    }

    void load();
    void save();
};

void applyFPS();
