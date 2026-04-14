#include "hooks.h"
#include "vars.h"
#include <MinHook.h>
#include <windows.h>
#include <cstdio>

uintptr_t gdBase = 0;
uintptr_t cocosBase = 0;

void InitDebugConsole() {
    AllocConsole();
    FILE* fDummy;
    freopen_s(&fDummy, "CONOUT$", "w", stdout);
    freopen_s(&fDummy, "CONOUT$", "w", stderr);
    freopen_s(&fDummy, "CONIN$", "r", stdin);
    printf("[Neverhook] Debug console initialized\n");
}

// оффсеты гд (расписал что где, шоб не забыть)
namespace Offsets {
    // GeometryDash.exe
    constexpr uintptr_t destroyPlayer = 0x3B39D0;
    constexpr uintptr_t playDeathEffect = 0x37ef10;
    constexpr uintptr_t togglePracticeMode = 0x3b9e50;

    // libcocos2d.dll
    constexpr uintptr_t schedulerUpdate = 0xC2B60;
}

static bool SafeHook(uintptr_t addr, LPVOID hook, LPVOID* original, const char* name) {
    if (addr == 0) {
        printf("[Neverhook] Hook skipped: %s (offset not set)\n", name);
        return false;
    }
    if (IsBadReadPtr((LPVOID)addr, 1)) {
        printf("[Neverhook] Hook failed: %s (bad address: %llX)\n", name, addr);
        return false;
    }
    if (MH_CreateHook((LPVOID)addr, hook, original) != MH_OK) {
        printf("[Neverhook] Hook failed: %s (MH_CreateHook failed)\n", name);
        return false;
    }
    if (MH_EnableHook((LPVOID)addr) != MH_OK) {
        printf("[Neverhook] Hook failed: %s (MH_EnableHook failed)\n", name);
        return false;
    }
    printf("[Neverhook] Hook success: %s at %llX\n", name, addr);
    return true;
}

typedef void(__fastcall* tDestroyPlayer)(void* self, void* player);
tDestroyPlayer oDestroyPlayer = nullptr;

void __fastcall hkDestroyPlayer(void* self, void* player) {
    if (Vars::noclip) {
        // printf("[Neverhook] Noclip blocked death\n"); // короче я просто вырежу его и все я так подумал
        return;
    }
    oDestroyPlayer(self, player);
}

typedef void(__fastcall* tPlayDeathEffect)(void* self);
tPlayDeathEffect oPlayDeathEffect = nullptr;

void __fastcall hkPlayDeathEffect(void* self) {
    if (Vars::noDeathEffect) {
        printf("[Neverhook] - No Death Effect - blocked effect\n");
        return;
    }
    oPlayDeathEffect(self);
}

typedef void(__fastcall* tTogglePracticeMode)(void* self, bool practiceMode);
tTogglePracticeMode oTogglePracticeMode = nullptr;

void __fastcall hkTogglePracticeMode(void* self, bool practiceMode) {
    if (Vars::practiceMusic) {
        printf("[Neverhook] Practice Music - Practice mode toggled\n"); // Вот тут я ваще не ебу почему не работает, мб надо через фмод делать
    }
    oTogglePracticeMode(self, practiceMode);
}

typedef void(__fastcall* tSchedulerUpdate)(void* self, float dt);
tSchedulerUpdate oSchedulerUpdate = nullptr;

void __fastcall hkSchedulerUpdate(void* self, float dt) {
    if (Vars::speedhack) {
        dt *= Vars::speedhackValue;
    }
    oSchedulerUpdate(self, dt);
}

void InitHooks() {
    InitDebugConsole();

    gdBase = (uintptr_t)GetModuleHandleA("GeometryDash.exe");
    cocosBase = (uintptr_t)GetModuleHandleA("libcocos2d.dll");

    printf("[Neverhook] GD Base: %llX\n", gdBase);
    printf("[Neverhook] Cocos Base: %llX\n", cocosBase);

    if (!gdBase || !cocosBase) {
        printf("[Neverhook] Failed to get module handles\n");
        return;
    }

    if (MH_Initialize() != MH_OK) {
        printf("[Neverhook] MinHook initialization failed\n");
        return;
    }
    printf("[Neverhook] MinHook initialized\n");

    SafeHook(gdBase + Offsets::destroyPlayer, &hkDestroyPlayer, (LPVOID*)&oDestroyPlayer, "destroyPlayer");
    SafeHook(gdBase + Offsets::playDeathEffect, &hkPlayDeathEffect, (LPVOID*)&oPlayDeathEffect, "playDeathEffect");
    SafeHook(gdBase + Offsets::togglePracticeMode, &hkTogglePracticeMode, (LPVOID*)&oTogglePracticeMode, "togglePracticeMode");

    SafeHook(cocosBase + Offsets::schedulerUpdate, &hkSchedulerUpdate, (LPVOID*)&oSchedulerUpdate, "schedulerUpdate");

    printf("[Neverhook] All hooks initialized\n");
}