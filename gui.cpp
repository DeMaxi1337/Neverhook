#include "gui.h"
#include "vars.h"
#include <imgui.h>

void DrawNeverhookMenu() {
    ImGui::SetNextWindowSize({ 450, 200 }, ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Neverhook", nullptr, ImGuiWindowFlags_NoCollapse)) {
        ImGui::Text("Build: v1.0.0-development");
        ImGui::Separator();

        if (ImGui::BeginChild("Hacks", ImVec2(0, 0), true)) {
            ImGui::Checkbox("NoClip", &Vars::noclip);
            ImGui::EndChild();
        }
    }
    ImGui::End();
}