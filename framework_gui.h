#pragma once

// -----------------------------------------------------------------------------
// framework_gui.h
//
// Public API of the neverlose-last GUI ported into Neverhook.
//
// Call order from main.cpp / hkwglSwapBuffers:
//
//   1) Right after the ImGui backends are initialised (still inside the
//      `if (!init)` block):
//
//          ImGui::CreateContext();
//          ImGui_ImplWin32_Init(window);
//          ImGui_ImplOpenGL3_Init("#version 130");
//          FrameWorkInit();                       // <-- adds custom fonts
//
//   2) Every frame, between NewFrame() and Render():
//
//          ImGui::NewFrame();
//          DrawNeverhookMenu();                   // existing menu (optional)
//          DrawFrameWorkGUI();                    // <-- ported framework
//          ImGui::Render();
//
//   3) Once, in DllMain DLL_PROCESS_DETACH, BEFORE shutting down the
//      ImGui backends:
//
//          FrameWorkShutdown();                   // <-- releases GL resources
//          ImGui_ImplOpenGL3_Shutdown();
//          ImGui_ImplWin32_Shutdown();
//          ImGui::DestroyContext();
//
// FrameWorkInit / FrameWorkShutdown are intentionally idempotent / safe
// to call once. They currently only manage fonts; the blur subsystem will
// hook into them in phase 2.
// -----------------------------------------------------------------------------

void FrameWorkInit();
void FrameWorkShutdown();
void DrawFrameWorkGUI();
