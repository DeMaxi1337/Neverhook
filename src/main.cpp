#include <Geode/Geode.hpp>
#include <Geode/modify/CCDirector.hpp>
#include "Config.hpp"
#include "gui/binds.h"

using namespace geode::prelude;

static const char* kKeys[] = {
    "noclip", "noDeathEffect", "noRespawnFlash", "practiceMusic",
    "iconBypass", "speedhack", "speedhackValue", "fpsUnlock",
    "fpsValue", "verifyHack"
};

void Config::load() {
    auto m = Mod::get();
    noclip         = m->getSavedValue<bool>("noclip", false);
    noclipTint        = m->getSavedValue<bool>("noclipTint", false);
    noclipTintColor[0]= (float)m->getSavedValue<double>("noclipTintColor0", 1.00);
    noclipTintColor[1]= (float)m->getSavedValue<double>("noclipTintColor1", 0.10);
    noclipTintColor[2]= (float)m->getSavedValue<double>("noclipTintColor2", 0.10);
    noclipTintColor[3]= (float)m->getSavedValue<double>("noclipTintColor3", 1.00);
    noclipTintOpacity = (float)m->getSavedValue<double>("noclipTintOpacity", 90.0);
    noclipTintTime    = (float)m->getSavedValue<double>("noclipTintTime", 0.50);
    noclipHitsound    = m->getSavedValue<bool>("noclipHitsound", false);
    noclipHitsoundVolume = (float)m->getSavedValue<double>("noclipHitsoundVolume", 100.0);
    noDeathEffect  = m->getSavedValue<bool>("noDeathEffect", false);
    noRespawnFlash = m->getSavedValue<bool>("noRespawnFlash", false);
    practiceMusic  = m->getSavedValue<bool>("practiceMusic", false);
    iconBypass     = m->getSavedValue<bool>("iconBypass", false);
    speedhack      = m->getSavedValue<bool>("speedhack", false);
    speedhackValue = (float)m->getSavedValue<double>("speedhackValue", 1.0);
    fpsUnlock      = m->getSavedValue<bool>("fpsUnlock", false);
    fpsValue       = (float)m->getSavedValue<double>("fpsValue", 240.0);
    verifyHack     = m->getSavedValue<bool>("verifyHack", false);
    copyHack       = m->getSavedValue<bool>("copyHack", false);
    hideAttempts          = m->getSavedValue<bool>("hideAttempts", false);
    accuratePercent       = m->getSavedValue<bool>("accuratePercent", false);
    accuratePercentDigits = m->getSavedValue<int>("accuratePercentDigits", 2);
    noGlow                = m->getSavedValue<bool>("noGlow", false);
    noCameraShake         = m->getSavedValue<bool>("noCameraShake", false);
    showHitboxes          = m->getSavedValue<bool>("showHitboxes", false);
    showHitboxesOnDeath   = m->getSavedValue<bool>("showHitboxesOnDeath", false);
    tpsBypass             = m->getSavedValue<bool>("tpsBypass", false);
    tpsValue              = (float)m->getSavedValue<double>("tpsValue", 240.0);
    // compactLists          = m->getSavedValue<bool>("compactLists", false);
    noMirrorPortal        = m->getSavedValue<bool>("noMirrorPortal", false);
    instantComplete       = m->getSavedValue<bool>("instantComplete", false);
    instantRestart        = m->getSavedValue<bool>("instantRestart", false);
    customRespawn         = m->getSavedValue<bool>("customRespawn", false);
    respawnTime           = (float)m->getSavedValue<double>("respawnTime", 1.0);
    safeMode              = m->getSavedValue<bool>("safeMode", false);
    safeFreezeAttempts    = m->getSavedValue<bool>("safeFreezeAttempts", true);
    safeFreezeJumps       = m->getSavedValue<bool>("safeFreezeJumps", true);

    noDashFire        = m->getSavedValue<bool>("noDashFire", false);
    noSpiderDash      = m->getSavedValue<bool>("noSpiderDash", false);
    noWavePulse       = m->getSavedValue<bool>("noWavePulse", false);
    noWaveTrail       = m->getSavedValue<bool>("noWaveTrail", false);
    solidWaveTrail    = m->getSavedValue<bool>("solidWaveTrail", false);
    waveTrailSize     = m->getSavedValue<bool>("waveTrailSize", false);
    waveTrailSizeValue = (float)m->getSavedValue<double>("waveTrailSizeValue", 1.0);
    noParticles       = m->getSavedValue<bool>("noParticles", false);
    noTrail           = m->getSavedValue<bool>("noTrail", false);

    hideEditorUI       = m->getSavedValue<bool>("hideEditorUI", false);
    levelEdit          = m->getSavedValue<bool>("levelEdit", false);
    noCustomObjLimit   = m->getSavedValue<bool>("noCustomObjLimit", false);
    noZoomLimit        = m->getSavedValue<bool>("noZoomLimit", false);
    toolboxButtonBypass = m->getSavedValue<bool>("toolboxButtonBypass", false);
    sliderLimitBypass  = m->getSavedValue<bool>("sliderLimitBypass", false);

    jumpHack          = m->getSavedValue<bool>("jumpHack", false);
    allModesPlatformer = m->getSavedValue<bool>("allModesPlatformer", false);
    hitboxMultiplier  = m->getSavedValue<bool>("hitboxMultiplier", false);
    hitboxMultPlayer  = (float)m->getSavedValue<double>("hitboxMultPlayer", 1.0);
    hitboxMultSolid   = (float)m->getSavedValue<double>("hitboxMultSolid", 1.0);
    hitboxMultHazard  = (float)m->getSavedValue<double>("hitboxMultHazard", 1.0);

    autoclicker      = m->getSavedValue<bool>("autoclicker", false);
    autoclickerP2    = m->getSavedValue<bool>("autoclickerP2", false);
    autoclickerCps   = (float)m->getSavedValue<double>("autoclickerCps", 10.0);
    autoclickerP2Cps = (float)m->getSavedValue<double>("autoclickerP2Cps", 10.0);

    noTransition   = m->getSavedValue<bool>("noTransition",   false);
    noPauseButton  = m->getSavedValue<bool>("noPauseButton",  false);
    speedhackAudio = m->getSavedValue<bool>("speedhackAudio", false);

    watermark       = m->getSavedValue<bool>("watermark",      false);
    watermarkStyle  = m->getSavedValue<int>("watermarkStyle",  0);
    watermarkLine   = m->getSavedValue<int>("watermarkLine",   1);
    watermarkPos    = m->getSavedValue<int>("watermarkPos",    2);
    wmGlow          = m->getSavedValue<bool>("wmGlow",         true);
    wmGlobal        = m->getSavedValue<bool>("wmGlobal",       true);
    wmBackground    = m->getSavedValue<bool>("wmBackground",   true);
    wmShowName      = m->getSavedValue<bool>("wmShowName",     true);
    wmShowUser      = m->getSavedValue<bool>("wmShowUser",     true);
    wmShowFps       = m->getSavedValue<bool>("wmShowFps",      true);
    wmShowTime      = m->getSavedValue<bool>("wmShowTime",     true);
    wmShowNcAcc     = m->getSavedValue<bool>("wmShowNcAcc", false);
    wmShowNcDeaths  = m->getSavedValue<bool>("wmShowNcDeaths", false);
    wmNameColorMode = m->getSavedValue<int>("wmNameColorMode", 0);
    wmColorOne[0]   = (float)m->getSavedValue<double>("wmColorOne0", 0.30);
    wmColorOne[1]   = (float)m->getSavedValue<double>("wmColorOne1", 0.49);
    wmColorOne[2]   = (float)m->getSavedValue<double>("wmColorOne2", 1.00);
    wmColorOne[3]   = (float)m->getSavedValue<double>("wmColorOne3", 1.00);
    wmColorTwo[0]   = (float)m->getSavedValue<double>("wmColorTwo0", 0.65);
    wmColorTwo[1]   = (float)m->getSavedValue<double>("wmColorTwo1", 0.80);
    wmColorTwo[2]   = (float)m->getSavedValue<double>("wmColorTwo2", 1.00);
    wmColorTwo[3]   = (float)m->getSavedValue<double>("wmColorTwo3", 1.00);

    keybindsList    = m->getSavedValue<bool>("keybindsList",  false);
    keybindsStyle   = m->getSavedValue<int>("keybindsStyle",  0);
    keybindsLine    = m->getSavedValue<int>("keybindsLine",   1);
    keybindsPos     = m->getSavedValue<int>("keybindsPos",    0);
    kbGlow          = m->getSavedValue<bool>("kbGlow",        true);
    kbGlobal        = m->getSavedValue<bool>("kbGlobal",      true);
    kbBackground    = m->getSavedValue<bool>("kbBackground",  true);
    keybindsX       = (float)m->getSavedValue<double>("keybindsX", 0.012);
    keybindsY       = (float)m->getSavedValue<double>("keybindsY", 0.045);
}

void Config::save() {
    auto m = Mod::get();
    m->setSavedValue("noclip", noclip);
    m->setSavedValue("noclipTint", noclipTint);
    m->setSavedValue("noclipTintColor0", (double)noclipTintColor[0]);
    m->setSavedValue("noclipTintColor1", (double)noclipTintColor[1]);
    m->setSavedValue("noclipTintColor2", (double)noclipTintColor[2]);
    m->setSavedValue("noclipTintColor3", (double)noclipTintColor[3]);
    m->setSavedValue("noclipTintOpacity", (double)noclipTintOpacity);
    m->setSavedValue("noclipTintTime", (double)noclipTintTime);
    m->setSavedValue("noclipHitsound", noclipHitsound);
    m->setSavedValue("noclipHitsoundVolume", (double)noclipHitsoundVolume);
    m->setSavedValue("noDeathEffect", noDeathEffect);
    m->setSavedValue("noRespawnFlash", noRespawnFlash);
    m->setSavedValue("practiceMusic", practiceMusic);
    m->setSavedValue("iconBypass", iconBypass);
    m->setSavedValue("speedhack", speedhack);
    m->setSavedValue("speedhackValue", (double)speedhackValue);
    m->setSavedValue("fpsUnlock", fpsUnlock);
    m->setSavedValue("fpsValue", (double)fpsValue);
    m->setSavedValue("verifyHack", verifyHack);
    m->setSavedValue("copyHack", copyHack);
    m->setSavedValue("hideAttempts", hideAttempts);
    m->setSavedValue("accuratePercent", accuratePercent);
    m->setSavedValue("accuratePercentDigits", accuratePercentDigits);
    m->setSavedValue("noGlow", noGlow);
    m->setSavedValue("noCameraShake", noCameraShake);
    m->setSavedValue("showHitboxes", showHitboxes);
    m->setSavedValue("showHitboxesOnDeath", showHitboxesOnDeath);
    m->setSavedValue("tpsBypass", tpsBypass);
    m->setSavedValue("tpsValue", (double)tpsValue);
    // m->setSavedValue("compactLists", compactLists);
    m->setSavedValue("noMirrorPortal", noMirrorPortal);
    m->setSavedValue("instantComplete", instantComplete);
    m->setSavedValue("instantRestart", instantRestart);
    m->setSavedValue("customRespawn", customRespawn);
    m->setSavedValue("respawnTime", (double)respawnTime);
    m->setSavedValue("safeMode", safeMode);
    m->setSavedValue("safeFreezeAttempts", safeFreezeAttempts);
    m->setSavedValue("safeFreezeJumps", safeFreezeJumps);

    m->setSavedValue("noDashFire", noDashFire);
    m->setSavedValue("noSpiderDash", noSpiderDash);
    m->setSavedValue("noWavePulse", noWavePulse);
    m->setSavedValue("noWaveTrail", noWaveTrail);
    m->setSavedValue("solidWaveTrail", solidWaveTrail);
    m->setSavedValue("waveTrailSize", waveTrailSize);
    m->setSavedValue("waveTrailSizeValue", (double)waveTrailSizeValue);
    m->setSavedValue("noParticles", noParticles);
    m->setSavedValue("noTrail", noTrail);

    m->setSavedValue("hideEditorUI", hideEditorUI);
    m->setSavedValue("levelEdit", levelEdit);
    m->setSavedValue("noCustomObjLimit", noCustomObjLimit);
    m->setSavedValue("noZoomLimit", noZoomLimit);
    m->setSavedValue("toolboxButtonBypass", toolboxButtonBypass);
    m->setSavedValue("sliderLimitBypass", sliderLimitBypass);

    m->setSavedValue("jumpHack", jumpHack);
    m->setSavedValue("allModesPlatformer", allModesPlatformer);
    m->setSavedValue("hitboxMultiplier", hitboxMultiplier);
    m->setSavedValue("hitboxMultPlayer", (double)hitboxMultPlayer);
    m->setSavedValue("hitboxMultSolid", (double)hitboxMultSolid);
    m->setSavedValue("hitboxMultHazard", (double)hitboxMultHazard);

    m->setSavedValue("autoclicker", autoclicker);
    m->setSavedValue("autoclickerP2", autoclickerP2);
    m->setSavedValue("autoclickerCps", (double)autoclickerCps);
    m->setSavedValue("autoclickerP2Cps", (double)autoclickerP2Cps);

    m->setSavedValue("noTransition",   noTransition);
    m->setSavedValue("noPauseButton",  noPauseButton);
    m->setSavedValue("speedhackAudio", speedhackAudio);

    m->setSavedValue("watermark",      watermark);
    m->setSavedValue("watermarkStyle", watermarkStyle);
    m->setSavedValue("watermarkLine",  watermarkLine);
    m->setSavedValue("watermarkPos",   watermarkPos);
    m->setSavedValue("wmGlow",         wmGlow);
    m->setSavedValue("wmGlobal",       wmGlobal);
    m->setSavedValue("wmBackground",   wmBackground);
    m->setSavedValue("wmShowName",     wmShowName);
    m->setSavedValue("wmShowUser",     wmShowUser);
    m->setSavedValue("wmShowFps",      wmShowFps);
    m->setSavedValue("wmShowTime",     wmShowTime);
    m->setSavedValue("wmShowNcAcc", wmShowNcAcc);
    m->setSavedValue("wmShowNcDeaths", wmShowNcDeaths);
    m->setSavedValue("wmNameColorMode",wmNameColorMode);
    m->setSavedValue("wmColorOne0",    (double)wmColorOne[0]);
    m->setSavedValue("wmColorOne1",    (double)wmColorOne[1]);
    m->setSavedValue("wmColorOne2",    (double)wmColorOne[2]);
    m->setSavedValue("wmColorOne3",    (double)wmColorOne[3]);
    m->setSavedValue("wmColorTwo0",    (double)wmColorTwo[0]);
    m->setSavedValue("wmColorTwo1",    (double)wmColorTwo[1]);
    m->setSavedValue("wmColorTwo2",    (double)wmColorTwo[2]);
    m->setSavedValue("wmColorTwo3",    (double)wmColorTwo[3]);

    m->setSavedValue("keybindsList",  keybindsList);
    m->setSavedValue("keybindsStyle", keybindsStyle);
    m->setSavedValue("keybindsLine",  keybindsLine);
    m->setSavedValue("keybindsPos",   keybindsPos);
    m->setSavedValue("kbGlow",        kbGlow);
    m->setSavedValue("kbGlobal",      kbGlobal);
    m->setSavedValue("kbBackground",  kbBackground);
    m->setSavedValue("keybindsX",     (double)keybindsX);
    m->setSavedValue("keybindsY",     (double)keybindsY);
}

void applyFPS() {
    auto& c = Config::get();
    double fps = std::clamp((double)c.fpsValue, 30.0, 1000.0);
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

// Geode 5.x has no $on_mod(Unloaded); autosave on every scene switch instead.
class $modify(NHDirectorSave, CCDirector) {
    void willSwitchToScene(CCScene* scene) {
        CCDirector::willSwitchToScene(scene);
        Config::get().save();
        BindSystem::get().save(Mod::get());
    }
};
