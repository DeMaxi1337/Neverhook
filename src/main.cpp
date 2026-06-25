#include <Geode/Geode.hpp>
#include "Config.hpp"
#include <Geode/modify/CCDirector.hpp>

using namespace geode::prelude;

static const char* kKeys[] = {
    "noclip", "noDeathEffect", "noRespawnFlash", "practiceMusic",
    "iconBypass", "speedhack", "speedhackValue", "fpsUnlock",
    "fpsValue", "verifyHack"
};

void Config::load() {
    auto m = Mod::get();
    noclip         = m->getSavedValue<bool>("noclip", false);
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
}

void Config::save() {
    auto m = Mod::get();
    m->setSavedValue("noclip", noclip);
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
    applyFPS();
    log::info("Neverhook loaded - press INSERT in game to open the menu");
}

class $modify(NHDirectorSave, CCDirector) {
    void end() {
        Config::get().save();
        CCDirector::end();
    }
};
