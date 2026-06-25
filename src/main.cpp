#include <Geode/Geode.hpp>
#include "Config.hpp"

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
