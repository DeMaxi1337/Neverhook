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
#pragma comment(lib, "deps\\minhook\\lib\\libMinHook.x64.lib")

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

WNDPROC oWndProc = nullptr;

typedef BOOL(WINAPI* twglSwapBuffers)(HDC hdc);
twglSwapBuffers owglSwapBuffers = nullptr;

LRESULT CALLBACK hkWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_KEYUP && wParam == VK_INSERT) {
        Vars::menuOpen = !Vars::menuOpen;
        ImGui::GetIO().MouseDrawCursor = Vars::menuOpen;
        return 0;
    }

    if (Vars::menuOpen) {
        ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam);

        if (msg >= WM_MOUSEFIRST && msg <= WM_MOUSELAST)
            return 1;

        if (msg == WM_KEYDOWN || msg == WM_KEYUP || msg == WM_CHAR)
            return 1;
    }

    return CallWindowProc(oWndProc, hWnd, msg, wParam, lParam);
}

BOOL WINAPI hkwglSwapBuffers(HDC hdc)
{
    static bool init = false;

    if (!init) {
        HWND window = WindowFromDC(hdc);
        if (window) {
            oWndProc = (WNDPROC)SetWindowLongPtr(window, GWLP_WNDPROC, (LONG_PTR)hkWndProc);

            ImGui::CreateContext();
            ImGui_ImplWin32_Init(window);
            ImGui_ImplOpenGL3_Init("#version 130");

            init = true;
        }
    }

    if (!Vars::menuOpen)
        return owglSwapBuffers ? owglSwapBuffers(hdc) : FALSE;

    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glPushClientAttrib(GL_CLIENT_ALL_ATTRIB_BITS);

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    DrawNeverhookMenu();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glPopClientAttrib();
    glPopAttrib();

    return owglSwapBuffers ? owglSwapBuffers(hdc) : FALSE;
}

void MainThread(HMODULE hModule)
{
    while (!GetModuleHandleA("libcocos2d.dll")) Sleep(100);

    InitHooks();

    HMODULE hOpenGL = GetModuleHandleA("opengl32.dll");
    if (!hOpenGL)
        return;

    void* swapAddr = GetProcAddress(hOpenGL, "wglSwapBuffers");
    if (!swapAddr)
        return;

    if (MH_CreateHook(swapAddr, &hkwglSwapBuffers, (LPVOID*)&owglSwapBuffers) == MH_OK)
        MH_EnableHook(swapAddr);
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID reserved)
{
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
        std::thread(MainThread, hModule).detach();
    }
    else if (reason == DLL_PROCESS_DETACH) {
        MH_DisableHook(MH_ALL_HOOKS);
        MH_Uninitialize();

        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();

        if (oWndProc) {
            HWND window = FindWindowA(nullptr, "Geometry Dash");
            if (window)
                SetWindowLongPtr(window, GWLP_WNDPROC, (LONG_PTR)oWndProc);
        }
    }
    return TRUE;
}