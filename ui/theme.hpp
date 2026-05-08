#pragma once

#include <imgui.h>

namespace stc::ui::theme {

// Applies a dark theme with rounded corners, restrained borders, and a single accent color.
// Call once after ImGui::CreateContext() and after platform/renderer init.
void apply_dark(ImVec4 accent = ImVec4(0.31F, 0.69F, 1.00F, 1.0F));

}  // namespace stc::ui::theme
