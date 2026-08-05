#pragma once

#include <string>
#include <vector>

struct lua_State;

namespace nh::lua {

enum class HookId {
    PlayLayerInit,
    PlayLayerReset,
    PlayLayerDestroyPlayer,
    PlayLayerLevelComplete,
    PlayLayerTogglePractice,
    GameLayerUpdate,
    GameLayerHandleButton,
    GameLayerResetVariables,
    PlayerPushButton,
    PlayerReleaseButton,
    PlayerDeathEffect,
    PlayerSpawnEffect,
    MenuLayerInit,
    PauseLayerSetup,
    EditorPauseLayerSetup,
    EndLevelLayerSetup,
    LevelInfoLayerInit,
    EditorUiInit,
    PlayLayerPostUpdate,
    PlayLayerResetFromStart,
    PlayLayerFullReset,
    PlayLayerPauseGame,
    PlayLayerOnQuit,
    PlayLayerCompleteEffect,
    PlayLayerStoreCheckpoint,
    PlayLayerLoadCheckpoint,
    PlayLayerRemoveCheckpoint,
    PlayLayerEndAnimation,
    PlayLayerAddObject,
    GameLayerProcessButtons,
    GameLayerTouchedRing,
    GameLayerTouchedTrigger,
    GameLayerGameEvent,
    GameLayerShakeCamera,
    PlayerUpdate,
    PlayerIncrementJumps,
    PlayerSpiderDash,
    PlayerRingJump,
    EditorPostUpdate,
    UiLayerInit,
    KeyboardMessage,
    LevelInfoDownloadFinished,
    LevelInfoEnterFinished,
    CreatorLayerInit,
    GameManagerInit,
    AppForeground,
    MusicFadeOut,
    ObjectShine,
    EffectTrigger,
    StreakPoint,
    Count,
};

struct HookDef {
    const char* name;
    bool        cancelable;
};

const HookDef& hookDef(HookId id);
bool           hookFromName(const char* name, HookId& out);
int            hookCount();

class HookEvent {
public:
    HookEvent& setNumber(const char* key, double value);
    HookEvent& setBool(const char* key, bool value);
    HookEvent& setString(const char* key, const std::string& value);

    void pushTable(lua_State* L) const;

private:
    enum class Kind { Number, Bool, String };

    struct Field {
        const char* key     = "";
        Kind        kind    = Kind::Number;
        double      number  = 0.0;
        bool        boolean = false;
        std::string text;
    };

    std::vector<Field> m_fields;
};

bool dispatchHook(HookId id, const HookEvent& event);
bool dispatchHook(HookId id);

bool hookHasListeners(HookId id);

void markHookListenersDirty();

void resetHookFrames();

void* hooksRegistryKey();
void  registerHooksApi(lua_State* L);

}
