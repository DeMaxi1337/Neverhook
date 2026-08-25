#pragma once

#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif

#include "imgui.h"

namespace nh::blur {

void newFrame();
void submit(ImDrawList* list, ImVec2 min, ImVec2 max, float rounding, float radius, float alpha);
bool available();

}
