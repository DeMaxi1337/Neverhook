#include "gui.h"
#include "hooks.h"
#include "vars.h"
#include <imgui.h>

void DrawNeverhookMenu() {
    ImGui::SetNextWindowSize({ 450, 300 }, ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Neverhook", nullptr, ImGuiWindowFlags_NoCollapse)) {
        ImGui::Text("Build: v1.0.5-development");
        ImGui::Separator();

        if (ImGui::BeginChild("Hacks", ImVec2(0, 0), true)) {

            if (ImGui::CollapsingHeader("Player", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::Checkbox("Noclip", &Vars::noclip);
                ImGui::Checkbox("No Death Effect", &Vars::noDeathEffect);
                ImGui::Checkbox("No Respawn Flash", &Vars::noRespawnFlash);
            }

            if (ImGui::CollapsingHeader("Bypass")) {
                ImGui::Checkbox("Practice Music", &Vars::practiceMusic);
                ImGui::Checkbox("Icon Bypass", &Vars::iconBypass);
            }

            if (ImGui::CollapsingHeader("Global")) {
                ImGui::Checkbox("Speedhack", &Vars::speedhack);
                if (Vars::speedhack) {
                    ImGui::SliderFloat("Speed", &Vars::speedhackValue, 0.1f, 5.0f, "%.2fx");
                }

                ImGui::Separator();

                if (ImGui::Checkbox("FPS Bypass", &Vars::fpsUnlock)) {
                    ApplyFPS();
                }
                if (Vars::fpsUnlock) {
                    if (ImGui::SliderFloat("##fpsslider", &Vars::fpsValue, 30.0f, 360.0f, "%.0f FPS")) {
                        ApplyFPS();
                    }
                    ImGui::SameLine();
                    ImGui::SetNextItemWidth(80);
                    if (ImGui::InputFloat("##fpsinput", &Vars::fpsValue, 0.0f, 0.0f, "%.0f")) {
                        if (Vars::fpsValue < 1.0f) Vars::fpsValue = 1.0f;
                        if (Vars::fpsValue > 1000.0f) Vars::fpsValue = 1000.0f;
                        ApplyFPS();
                    }
                }
            }

            ImGui::EndChild();
        }
    }
    ImGui::End();
}