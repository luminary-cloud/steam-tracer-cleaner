#pragma once

#include <imgui.h>

namespace stc::ui::fonts {

// Loads Segoe UI from %WINDIR%/Fonts at startup. Call once after ImGui::CreateContext() and before
// ImGui_ImplDX11_Init. Falls back silently to the built-in proggy font if Segoe is missing.
void load();

ImFont* body();
ImFont* title();

}  // namespace stc::ui::fonts
