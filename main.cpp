#include <windows.h>
#include <thread>
#include <GL/gl.h>
#include <cstdio>
#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_opengl3.h>
#include <MinHook.h>

#include "vars.h"
#include "gui.h"
#include "hooks.h"
#include "framework_gui.h"
#include "framework_widgets.h"  // gui.m_fade

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

        if (Vars::menuOpen) {
            // Release any cursor-clipping the game might have set so the user
            // can drag the mouse freely over the menu.
            ClipCursor(NULL);

            // Snap the OS cursor to the window center so it doesn't "teleport"
            // when ImGui starts drawing its software cursor at the last known
            // io.MousePos (which in fullscreen GD is often a screen edge).
            RECT rc{};
            if (GetClientRect(hWnd, &rc)) {
                POINT c{ (rc.right - rc.left) / 2, (rc.bottom - rc.top) / 2 };
                ClientToScreen(hWnd, &c);
                SetCursorPos(c.x, c.y);
                ImGui::GetIO().MousePos = ImVec2((float)((rc.right - rc.left) / 2),
                                                 (float)((rc.bottom - rc.top) / 2));
            }
        }
        return 0;
    }

    // Block game input while the menu is visible OR still fading out
    const bool guiActive = Vars::menuOpen || gui.m_fade > 0.004f;
    if (guiActive) {
        ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam);

        // Mouse buttons / wheel / movement (WM_MOUSEFIRST..WM_MOUSELAST)
        if (msg >= WM_MOUSEFIRST && msg <= WM_MOUSELAST)
            return 1;

        // Standard keys + Alt-modified keys + char input
        if (msg == WM_KEYDOWN || msg == WM_KEYUP || msg == WM_CHAR ||
            msg == WM_SYSKEYDOWN || msg == WM_SYSKEYUP || msg == WM_SYSCHAR)
            return 1;

        // Raw Input (WM_INPUT) — cocos2d-x and many GD mods read raw mouse/kbd
        // through this message; if we don't drop it here, the game still sees
        // every click / keystroke even though the menu has eaten WM_MOUSE*.
        if (msg == WM_INPUT)
            return 0;
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
            FrameWorkInit();

            init = true;
        }
    }

    // Always run the ImGui loop so DrawFrameWorkGUI can animate the fade-out
    // even after menuOpen becomes false.
    const bool needsRender = Vars::menuOpen || gui.m_fade > 0.004f;
    if (!needsRender)
        return owglSwapBuffers ? owglSwapBuffers(hdc) : FALSE;

    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glPushClientAttrib(GL_CLIENT_ALL_ATTRIB_BITS);

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    // DrawNeverhookMenu();  // legacy menu; widgets migrated into DrawFrameWorkGUI tabs
    DrawFrameWorkGUI();

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

        FrameWorkShutdown();
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