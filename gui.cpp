#include "gui.h"
#include "vars.h"
#include <imgui.h>

void DrawNeverhookMenu() {
    ImGui::SetNextWindowSize({ 450, 300 }, ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Neverhook", nullptr, ImGuiWindowFlags_NoCollapse)) {
        ImGui::Text("Build: v1.0.1-development");
        ImGui::Separator();

        if (ImGui::BeginChild("Hacks", ImVec2(0, 0), true)) {

            if (ImGui::CollapsingHeader("Player", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::Checkbox("Noclip", &Vars::noclip);
                ImGui::Checkbox("No Death Effect", &Vars::noDeathEffect);
            }

            if (ImGui::CollapsingHeader("Bypass")) {
                ImGui::Checkbox("Practice Music", &Vars::practiceMusic);
            }

            if (ImGui::CollapsingHeader("Global")) {
                ImGui::Checkbox("Speedhack", &Vars::speedhack);
                if (Vars::speedhack) {
                    ImGui::SliderFloat("Speed", &Vars::speedhackValue, 0.1f, 5.0f, "%.2fx");
                }
            }

            ImGui::EndChild();
        }
    }
    ImGui::End();
}