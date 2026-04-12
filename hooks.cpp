#include "hooks.h"
#include "vars.h"
#include <MinHook.h>
#include <windows.h>
#include <cstdio>

typedef void(__fastcall* tDestroyPlayer)(void* self, void* p1, void* p2);
tDestroyPlayer oDestroyPlayer = nullptr;

void __fastcall hkDestroyPlayer(void* self, void* p1, void* p2) {
    if (Vars::noclip) {
        return;
    }
    oDestroyPlayer(self, p1, p2);
}

void InitHooks() {
    if (MH_Initialize() != MH_OK) {
        printf("[Neverhook] MinHook initialization failed!\n");
        return;
    }

    uintptr_t base = (uintptr_t)GetModuleHandleA(NULL);

    LPVOID noclipAddr = (LPVOID)(base + 0x3B39D0);

    if (MH_CreateHook(noclipAddr, (LPVOID)&hkDestroyPlayer, (LPVOID*)&oDestroyPlayer) == MH_OK) {
        MH_EnableHook(noclipAddr);
        printf("[Neverhook] Noclip hooked!\n");
    }
    else {
        printf("[Neverhook] Failed to hook Noclip!\n");
    }
}
