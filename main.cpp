#include <windows.h>
#include <thread>
#include <GL/gl.h>
#include <cstdio>
#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_opengl3.h>
#include <MinHook.h>

#include "vars.h"
#include "gui.h"
#include "hooks.h"

#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "D:\\Neverhook\\deps\\minhook\\lib\\libMinHook.x64.lib")

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

WNDPROC oWndProc = nullptr;
typedef BOOL(WINAPI* twglSwapBuffers)(HDC hdc);
twglSwapBuffers owglSwapBuffers = nullptr;

LRESULT CALLBACK hkWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_KEYDOWN && wParam == VK_INSERT) {
        Vars::menuOpen = !Vars::menuOpen;
        if (!Vars::menuOpen) {
            ImGui::GetIO().MouseDrawCursor = false;
        }
        return 0;
    }

    if (Vars::menuOpen) {
        ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam);

        if (msg >= WM_MOUSEFIRST && msg <= WM_MOUSELAST) return true;
        if (msg == WM_KEYDOWN || msg == WM_CHAR) return true;
    }

    return CallWindowProc(oWndProc, hWnd, msg, wParam, lParam);
}

BOOL WINAPI hkwglSwapBuffers(HDC hdc) {
    static bool init = false;
    if (!init) {
        HWND window = WindowFromDC(hdc);
        oWndProc = (WNDPROC)SetWindowLongPtr(window, GWLP_WNDPROC, (LONG_PTR)hkWndProc);
        ImGui::CreateContext();
        ImGui_ImplWin32_Init(window);
        ImGui_ImplOpenGL3_Init();
        init = true;
    }

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    if (Vars::menuOpen) {
        DrawNeverhookMenu();
    }

    ImGui::Render();
    if (ImGui::GetDrawData()) {
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }

    return owglSwapBuffers(hdc);
}

void MainThread(HMODULE hModule) {
    AllocConsole();
    freopen_s((FILE**)stdout, "CONOUT$", "w", stdout);

    Sleep(1000);

    InitHooks();

    void* swapAddr = GetProcAddress(GetModuleHandleA("opengl32.dll"), "wglSwapBuffers");
    if (swapAddr) {
        MH_CreateHook(swapAddr, &hkwglSwapBuffers, (LPVOID*)&owglSwapBuffers);
        MH_EnableHook(swapAddr);
    }
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID reserved) {
    if (reason == DLL_PROCESS_ATTACH) std::thread(MainThread, hModule).detach();
    return TRUE;
}