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
    constexpr uintptr_t isIconUnlocked = 0x17c500;
    constexpr uintptr_t isColorUnlocked = 0x17c8e0;
    constexpr uintptr_t isItemUnlocked = 0x1e5820;

    // libcocos2d.dll
    constexpr uintptr_t schedulerUpdate = 0xC2B60;
}

static bool SafeHook(uintptr_t addr, LPVOID hook, LPVOID* original, const char* name) {
    if (addr == 0) {
        printf("[!] Hook skipped: %s (offset not set)\n", name);
        return false;
    }
    if (IsBadReadPtr((LPVOID)addr, 1)) {
        printf("[!] Hook failed: %s (bad address: %llX)\n", name, addr);
        return false;
    }
    if (MH_CreateHook((LPVOID)addr, hook, original) != MH_OK) {
        printf("[!] Hook failed: %s (MH_CreateHook failed)\n", name);
        return false;
    }
    if (MH_EnableHook((LPVOID)addr) != MH_OK) {
        printf("[!] Hook failed: %s (MH_EnableHook failed)\n", name);
        return false;
    }
    printf("[+] Hook success: %s at %llX\n", name, addr);
    return true;
}

typedef void(__fastcall* tDestroyPlayer)(void* self, void* player);
tDestroyPlayer oDestroyPlayer = nullptr;

void __fastcall hkDestroyPlayer(void* self, void* player) {
    if (Vars::noclip) return;
    oDestroyPlayer(self, player);
}

typedef void(__fastcall* tPlayDeathEffect)(void* self);
tPlayDeathEffect oPlayDeathEffect = nullptr;

void __fastcall hkPlayDeathEffect(void* self) {
    if (Vars::noDeathEffect) return;
    oPlayDeathEffect(self);
}

typedef void(__fastcall* tSchedulerUpdate)(void* self, float dt);
tSchedulerUpdate oSchedulerUpdate = nullptr;

void __fastcall hkSchedulerUpdate(void* self, float dt) {
    if (Vars::speedhack) {
        dt *= Vars::speedhackValue;
    }
    oSchedulerUpdate(self, dt);
}

typedef bool(__fastcall* tIsIconUnlocked)(void* self, int id, int type);
tIsIconUnlocked oIsIconUnlocked = nullptr;

bool __fastcall hkIsIconUnlocked(void* self, int id, int type) {
    if (Vars::iconBypass) return true;
    return oIsIconUnlocked(self, id, type);
}

typedef bool(__fastcall* tIsColorUnlocked)(void* self, int id, int type);
tIsColorUnlocked oIsColorUnlocked = nullptr;

bool __fastcall hkIsColorUnlocked(void* self, int id, int type) {
    if (Vars::iconBypass) return true;
    return oIsColorUnlocked(self, id, type);
}

typedef bool(__fastcall* tIsItemUnlocked)(void* self, int type, int id);
tIsItemUnlocked oIsItemUnlocked = nullptr;

bool __fastcall hkIsItemUnlocked(void* self, int type, int id) {
    if (Vars::practiceMusic && type == 12 && id == 17)
        return true;
    return oIsItemUnlocked(self, type, id);
}

void InitHooks() {
    InitDebugConsole();

    gdBase = (uintptr_t)GetModuleHandleA("GeometryDash.exe");
    cocosBase = (uintptr_t)GetModuleHandleA("libcocos2d.dll");

    printf("[*] GD Base: %llX\n", gdBase);
    printf("[*] Cocos Base: %llX\n", cocosBase);

    if (!gdBase || !cocosBase) {
        printf("[!] Failed to get module handles\n");
        return;
    }

    if (MH_Initialize() != MH_OK) {
        printf("[!] MinHook initialization failed\n");
        return;
    }
    printf("[+] MinHook initialized\n");

    SafeHook(gdBase + Offsets::destroyPlayer, &hkDestroyPlayer, (LPVOID*)&oDestroyPlayer, "destroyPlayer");
    SafeHook(gdBase + Offsets::playDeathEffect, &hkPlayDeathEffect, (LPVOID*)&oPlayDeathEffect, "playDeathEffect");
    SafeHook(gdBase + Offsets::isIconUnlocked, &hkIsIconUnlocked, (LPVOID*)&oIsIconUnlocked, "isIconUnlocked");
    SafeHook(gdBase + Offsets::isColorUnlocked, &hkIsColorUnlocked, (LPVOID*)&oIsColorUnlocked, "isColorUnlocked");
    SafeHook(gdBase + Offsets::isItemUnlocked, &hkIsItemUnlocked, (LPVOID*)&oIsItemUnlocked, "isItemUnlocked");

    SafeHook(cocosBase + Offsets::schedulerUpdate, &hkSchedulerUpdate, (LPVOID*)&oSchedulerUpdate, "schedulerUpdate");

    printf("[*] All hooks initialized\n");
}