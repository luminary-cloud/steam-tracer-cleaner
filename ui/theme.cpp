#include "ui/theme.hpp"

namespace stc::ui::theme {

void apply_dark(ImVec4 accent) {
    ImGuiStyle& s = ImGui::GetStyle();

    s.WindowRounding = 0.0F;
    s.ChildRounding = 6.0F;
    s.FrameRounding = 5.0F;
    s.PopupRounding = 6.0F;
    s.GrabRounding = 4.0F;
    s.ScrollbarRounding = 100.0F;
    s.TabRounding = 4.0F;

    s.WindowPadding = ImVec2(0, 0);
    s.FramePadding = ImVec2(8, 4);
    s.ItemSpacing = ImVec2(10, 8);
    s.ItemInnerSpacing = ImVec2(6, 4);
    s.IndentSpacing = 18.0F;
    s.ScrollbarSize = 6.0F;

    s.WindowBorderSize = 0.0F;
    s.ChildBorderSize = 0.0F;
    s.PopupBorderSize = 1.0F;
    s.FrameBorderSize = 0.0F;
    s.TabBorderSize = 0.0F;

    auto& c = s.Colors;
    c[ImGuiCol_Text] = ImVec4(0.95F, 0.96F, 0.97F, 1.00F);
    c[ImGuiCol_TextDisabled] = ImVec4(0.443F, 0.443F, 0.471F, 1.00F);

    c[ImGuiCol_WindowBg] = ImVec4(0.035F, 0.035F, 0.035F, 1.00F);
    c[ImGuiCol_ChildBg] = ImVec4(0.047F, 0.047F, 0.047F, 1.00F);
    c[ImGuiCol_PopupBg] = ImVec4(0.047F, 0.047F, 0.047F, 0.98F);
    c[ImGuiCol_Border] = ImVec4(0.659F, 0.635F, 0.620F, 0.10F);
    c[ImGuiCol_BorderShadow] = ImVec4(0.0F, 0.0F, 0.0F, 0.0F);

    c[ImGuiCol_FrameBg] = ImVec4(0.07F, 0.07F, 0.07F, 1.00F);
    c[ImGuiCol_FrameBgHovered] = ImVec4(0.090F, 0.090F, 0.090F, 1.00F);
    c[ImGuiCol_FrameBgActive] = ImVec4(0.118F, 0.118F, 0.118F, 1.00F);

    c[ImGuiCol_TitleBg] = ImVec4(0.035F, 0.035F, 0.035F, 1.00F);
    c[ImGuiCol_TitleBgActive] = ImVec4(0.035F, 0.035F, 0.035F, 1.00F);
    c[ImGuiCol_TitleBgCollapsed] = ImVec4(0.035F, 0.035F, 0.035F, 0.80F);
    c[ImGuiCol_MenuBarBg] = ImVec4(0.047F, 0.047F, 0.047F, 1.00F);

    c[ImGuiCol_ScrollbarBg] = ImVec4(0.0F, 0.0F, 0.0F, 0.0F);
    c[ImGuiCol_ScrollbarGrab] = ImVec4(0.659F, 0.635F, 0.620F, 0.20F);
    c[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.659F, 0.635F, 0.620F, 0.35F);
    c[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.659F, 0.635F, 0.620F, 0.55F);

    c[ImGuiCol_CheckMark] = accent;
    c[ImGuiCol_SliderGrab] = accent;
    c[ImGuiCol_SliderGrabActive] = ImVec4(accent.x, accent.y, accent.z, 1.0F);

    c[ImGuiCol_Button] = ImVec4(0.090F, 0.090F, 0.090F, 1.00F);
    c[ImGuiCol_ButtonHovered] = ImVec4(0.118F, 0.118F, 0.118F, 1.00F);
    c[ImGuiCol_ButtonActive] = ImVec4(accent.x * 0.55F, accent.y * 0.55F, accent.z * 0.55F, 1.0F);

    c[ImGuiCol_Header] = ImVec4(0.080F, 0.080F, 0.080F, 1.00F);
    c[ImGuiCol_HeaderHovered] = ImVec4(0.110F, 0.110F, 0.110F, 1.00F);
    c[ImGuiCol_HeaderActive] = ImVec4(0.140F, 0.140F, 0.140F, 1.00F);

    c[ImGuiCol_Separator] = ImVec4(0.659F, 0.635F, 0.620F, 0.10F);
    c[ImGuiCol_SeparatorHovered] = ImVec4(0.659F, 0.635F, 0.620F, 0.20F);
    c[ImGuiCol_SeparatorActive] = accent;

    c[ImGuiCol_ResizeGrip] = ImVec4(0.0F, 0.0F, 0.0F, 0.0F);
    c[ImGuiCol_ResizeGripHovered] = ImVec4(accent.x, accent.y, accent.z, 0.40F);
    c[ImGuiCol_ResizeGripActive] = accent;

    c[ImGuiCol_Tab] = ImVec4(0.047F, 0.047F, 0.047F, 1.00F);
    c[ImGuiCol_TabHovered] = ImVec4(0.090F, 0.090F, 0.090F, 1.00F);
    c[ImGuiCol_TabActive] = ImVec4(0.070F, 0.070F, 0.070F, 1.00F);

    c[ImGuiCol_DockingPreview] = ImVec4(accent.x, accent.y, accent.z, 0.50F);
    c[ImGuiCol_DockingEmptyBg] = ImVec4(0.035F, 0.035F, 0.035F, 1.00F);

    c[ImGuiCol_PlotLines] = ImVec4(0.61F, 0.61F, 0.61F, 1.00F);
    c[ImGuiCol_PlotHistogram] = accent;

    c[ImGuiCol_TextSelectedBg] = ImVec4(accent.x, accent.y, accent.z, 0.35F);
    c[ImGuiCol_NavHighlight] = accent;
}

}  // namespace stc::ui::theme
