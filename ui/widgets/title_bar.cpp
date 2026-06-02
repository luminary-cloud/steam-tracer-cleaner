#include "ui/widgets/title_bar.hpp"

#include <windows.h>

#include <imgui.h>

#include "ui/fonts.hpp"
#include "ui/icons.hpp"

namespace stc::ui::widgets {

namespace {

constexpr char kAppName[] = "Steam Tracer Cleaner";

}  // namespace

void draw_title_bar() {
    const HWND hwnd = static_cast<HWND>(ImGui::GetMainViewport()->PlatformHandleRaw);
    const float full_w = ImGui::GetContentRegionAvail().x;

    ImGui::BeginChild("##titlebar", ImVec2(full_w, kTitleBarHeight), false,
                      ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    auto* draw = ImGui::GetWindowDrawList();
    const ImVec2 pos = ImGui::GetWindowPos();
    const ImVec2 end{pos.x + full_w, pos.y + kTitleBarHeight};
    const float cy = pos.y + kTitleBarHeight * 0.5F;

    const ImU32 bar_bg = ImColor(0.047F, 0.047F, 0.047F, 1.0F);
    const ImU32 bar_border = ImColor(0.659F, 0.635F, 0.620F, 0.10F);
    const ImU32 text_col = ImColor(0.95F, 0.96F, 0.97F, 1.0F);
    const ImU32 dim_col = ImColor(0.443F, 0.443F, 0.471F, 1.0F);

    draw->AddRectFilled(pos, end, bar_bg);
    draw->AddLine(ImVec2(pos.x, end.y - 1), ImVec2(end.x, end.y - 1), bar_border);

    float text_x = pos.x + 14;
    if (const ImTextureID app_ico = icons::app_icon()) {
        constexpr float kIconSize = 18.0F;
        draw->AddImage(app_ico, ImVec2(pos.x + 12, cy - kIconSize * 0.5F),
                       ImVec2(pos.x + 12 + kIconSize, cy + kIconSize * 0.5F));
        text_x = pos.x + 12 + kIconSize + 8;
    }

    if (auto* tf = fonts::title()) {
        ImGui::PushFont(tf);
    }
    const float text_h = ImGui::GetTextLineHeight();
    draw->AddText(ImVec2(text_x, pos.y + (kTitleBarHeight - text_h) * 0.5F), text_col, kAppName);
    if (fonts::title()) {
        ImGui::PopFont();
    }

    // One right-aligned control button. `slot` counts in from the right edge
    // (1 = close, 2 = maximize, 3 = minimize). Draws the hover fill and reports
    // hover state back so the caller can pick a glyph color; returns true on a
    // completed click.
    auto button = [&](const char* id, int slot, ImU32 hover_col, bool& hovered) {
        const float x1 = full_w - kCaptionBtnWidth * static_cast<float>(slot - 1);
        const float x0 = x1 - kCaptionBtnWidth;
        ImGui::SetCursorPos(ImVec2(x0, 0));
        ImGui::PushID(id);
        const bool clicked =
            ImGui::InvisibleButton("##cap", ImVec2(kCaptionBtnWidth, kTitleBarHeight));
        hovered = ImGui::IsItemHovered();
        ImGui::PopID();
        if (hovered) {
            draw->AddRectFilled(ImVec2(pos.x + x0, pos.y), ImVec2(pos.x + x1, end.y), hover_col);
        }
        return clicked;
    };

    const ImU32 light = ImColor(1.0F, 1.0F, 1.0F, 0.08F);
    auto glyph_col = [&](bool hovered) { return hovered ? text_col : dim_col; };

    bool hovered = false;

    const float min_cx = pos.x + full_w - kCaptionBtnWidth * 2.5F;
    if (button("min", 3, light, hovered) && hwnd) {
        ShowWindow(hwnd, SW_MINIMIZE);
    }
    draw->AddLine(ImVec2(min_cx - 5, cy), ImVec2(min_cx + 5, cy), glyph_col(hovered), 1.0F);

    const bool maximized = hwnd && IsZoomed(hwnd);
    const float max_cx = pos.x + full_w - kCaptionBtnWidth * 1.5F;
    if (button("max", 2, light, hovered) && hwnd) {
        ShowWindow(hwnd, maximized ? SW_RESTORE : SW_MAXIMIZE);
    }
    {
        const ImU32 c = glyph_col(hovered);
        if (maximized) {
            draw->AddRect(ImVec2(max_cx - 2, cy - 5), ImVec2(max_cx + 5, cy + 2), c);
            draw->AddRect(ImVec2(max_cx - 5, cy - 2), ImVec2(max_cx + 2, cy + 5), c);
        } else {
            draw->AddRect(ImVec2(max_cx - 5, cy - 5), ImVec2(max_cx + 5, cy + 5), c);
        }
    }

    const float cls_cx = pos.x + full_w - kCaptionBtnWidth * 0.5F;
    if (button("close", 1, ImColor(0.91F, 0.07F, 0.14F, 1.0F), hovered) && hwnd) {
        PostMessageW(hwnd, WM_CLOSE, 0, 0);
    }
    {
        const ImU32 c = glyph_col(hovered);
        draw->AddLine(ImVec2(cls_cx - 5, cy - 5), ImVec2(cls_cx + 5, cy + 5), c, 1.2F);
        draw->AddLine(ImVec2(cls_cx - 5, cy + 5), ImVec2(cls_cx + 5, cy - 5), c, 1.2F);
    }

    ImGui::EndChild();
}

}  // namespace stc::ui::widgets
