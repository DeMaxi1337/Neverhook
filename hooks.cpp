#include "hooks.h"
#include "vars.h"
#include <MinHook.h>
#include <windows.h>
#include <cstdio>
#include <psapi.h>
#pragma comment(lib, "psapi.lib")

uintptr_t gdBase = 0;
uintptr_t cocosBase = 0;

void InitDebugConsole() {
    AllocConsole();

    FILE* f;
    freopen_s(&f, "CONOUT$", "w", stdout);
    freopen_s(&f, "CONOUT$", "w", stderr);
    freopen_s(&f, "CONIN$", "r", stdin);

    printf("[Neverhook] Debug console initialized\n");
}

namespace Offsets {
    constexpr uintptr_t destroyPlayer = 0x3B39D0;
    constexpr uintptr_t playDeathEffect = 0x37ef10;
    constexpr uintptr_t isIconUnlocked = 0x17c500;
    constexpr uintptr_t isColorUnlocked = 0x17c8e0;
    constexpr uintptr_t isItemUnlocked = 0x1e5820;
    constexpr uintptr_t playSpawnEffect = 0x397480;

    constexpr uintptr_t schedulerUpdate = 0xC2B60;
    constexpr uintptr_t setAnimationInterval = 0xBF3F0;
}

static bool SafeHook(uintptr_t addr, LPVOID hook, LPVOID* original, const char* name) {
    if (!addr) {
        printf("[!] Hook skipped: %s (null address)\n", name);
        return false;
    }

    if (MH_CreateHook((LPVOID)addr, hook, original) != MH_OK) {
        printf("[!] Hook failed: %s (MH_CreateHook)\n", name);
        return false;
    }

    if (MH_EnableHook((LPVOID)addr) != MH_OK) {
        printf("[!] Hook failed: %s (MH_EnableHook)\n", name);
        return false;
    }

    printf("[+] Hook success: %s at %p\n", name, (void*)addr);
    return true;
}

typedef void(__fastcall* tDestroyPlayer)(void*, void*);
tDestroyPlayer oDestroyPlayer = nullptr;

void __fastcall hkDestroyPlayer(void* self, void* player) {
    if (Vars::noclip) return;
    if (oDestroyPlayer) oDestroyPlayer(self, player);
}

typedef void(__fastcall* tPlayDeathEffect)(void*);
tPlayDeathEffect oPlayDeathEffect = nullptr;

void __fastcall hkPlayDeathEffect(void* self) {
    if (Vars::noDeathEffect) return;
    if (oPlayDeathEffect) oPlayDeathEffect(self);
}

typedef void(__fastcall* tSchedulerUpdate)(void*, float);
tSchedulerUpdate oSchedulerUpdate = nullptr;

void __fastcall hkSchedulerUpdate(void* self, float dt) {
    if (Vars::speedhack)
        dt *= Vars::speedhackValue;

    if (oSchedulerUpdate)
        oSchedulerUpdate(self, dt);
}

typedef bool(__fastcall* tIsIconUnlocked)(void*, int, int);
tIsIconUnlocked oIsIconUnlocked = nullptr;

bool __fastcall hkIsIconUnlocked(void* self, int id, int type) {
    if (Vars::iconBypass) return true;
    return oIsIconUnlocked ? oIsIconUnlocked(self, id, type) : false;
}

typedef bool(__fastcall* tIsColorUnlocked)(void*, int, int);
tIsColorUnlocked oIsColorUnlocked = nullptr;

bool __fastcall hkIsColorUnlocked(void* self, int id, int type) {
    if (Vars::iconBypass) return true;
    return oIsColorUnlocked ? oIsColorUnlocked(self, id, type) : false;
}

typedef bool(__fastcall* tIsItemUnlocked)(void*, int, int);
tIsItemUnlocked oIsItemUnlocked = nullptr;

bool __fastcall hkIsItemUnlocked(void* self, int type, int id) {
    if (Vars::practiceMusic && type == 12 && id == 17)
        return true;

    return oIsItemUnlocked ? oIsItemUnlocked(self, type, id) : false;
}

typedef void* (__cdecl* tSharedDirector)();
typedef void(__thiscall* tSetAnimationInterval)(void*, double);

tSharedDirector pSharedDirector = nullptr;
tSetAnimationInterval pSetAnimationInterval = nullptr;

void ApplyFPS() {
    if (!pSharedDirector || !pSetAnimationInterval)
        return;

    void* director = pSharedDirector();
    if (!director)
        return;

    double interval = Vars::fpsUnlock
        ? (1.0 / (double)Vars::fpsValue)
        : (1.0 / 60.0);

    pSetAnimationInterval(director, interval);
}

typedef void(__thiscall* tPlaySpawnEffect)(void*);
tPlaySpawnEffect oPlaySpawnEffect = nullptr;

void __fastcall hkPlaySpawnEffect(void* self) {
    if (Vars::noRespawnFlash) return;
    if (oPlaySpawnEffect) oPlaySpawnEffect(self);
}

void InitHooks() {
    InitDebugConsole();

    gdBase = (uintptr_t)GetModuleHandleA("GeometryDash.exe");
    cocosBase = (uintptr_t)GetModuleHandleA("libcocos2d.dll");

    printf("[*] GD Base: %p\n", (void*)gdBase);
    printf("[*] Cocos Base: %p\n", (void*)cocosBase);

    if (!gdBase || !cocosBase)
        return;

    if (MH_Initialize() != MH_OK)
        return;

    pSharedDirector = (tSharedDirector)GetProcAddress(
        (HMODULE)cocosBase,
        "?sharedDirector@CCDirector@cocos2d@@SAPEAV12@XZ"
    );

    pSetAnimationInterval =
        (tSetAnimationInterval)(cocosBase + Offsets::setAnimationInterval);

    SafeHook(gdBase + Offsets::destroyPlayer, &hkDestroyPlayer, (LPVOID*)&oDestroyPlayer, "destroyPlayer");
    SafeHook(gdBase + Offsets::playDeathEffect, &hkPlayDeathEffect, (LPVOID*)&oPlayDeathEffect, "playDeathEffect");
    SafeHook(gdBase + Offsets::isIconUnlocked, &hkIsIconUnlocked, (LPVOID*)&oIsIconUnlocked, "isIconUnlocked");
    SafeHook(gdBase + Offsets::isColorUnlocked, &hkIsColorUnlocked, (LPVOID*)&oIsColorUnlocked, "isColorUnlocked");
    SafeHook(gdBase + Offsets::isItemUnlocked, &hkIsItemUnlocked, (LPVOID*)&oIsItemUnlocked, "isItemUnlocked");
    SafeHook(gdBase + Offsets::playSpawnEffect, &hkPlaySpawnEffect, (LPVOID*)&oPlaySpawnEffect, "playSpawnEffect");
    SafeHook(cocosBase + Offsets::schedulerUpdate, &hkSchedulerUpdate, (LPVOID*)&oSchedulerUpdate, "schedulerUpdate");

    printf("[*] Hooks initialized\n");
}