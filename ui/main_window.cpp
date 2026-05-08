#include "ui/main_window.hpp"

#include <imgui.h>

#include <array>

#include "ui/fonts.hpp"
#include "ui/screens/audit_screen.hpp"
#include "ui/screens/backups_screen.hpp"
#include "ui/screens/cleaner_screen.hpp"
#include "ui/screens/configs_screen.hpp"
#include "ui/screens/settings_screen.hpp"

namespace stc::ui {
namespace {

constexpr float kSidebarWidth = 188.0F;
constexpr float kSidebarPaddingX = 16.0F;
constexpr float kSidebarPaddingY = 18.0F;
constexpr float kNavItemHeight = 32.0F;
constexpr float kNavItemSpacing = 4.0F;
constexpr float kSectionGap = 18.0F;
constexpr float kStatusBarHeight = 36.0F;
constexpr float kFooterMargin = 16.0F;       // gap between scrollable content and the status bar
constexpr float kContentPaddingX = 24.0F;
constexpr float kContentPaddingY = 20.0F;

struct NavSection {
    const char* heading;
    struct Item {
        stc::app::Screen screen;
        const char* label;
        const char* hint;
    };
    std::initializer_list<Item> items;
};

const ImVec4 kAccent{0.31F, 0.69F, 1.00F, 1.0F};
const ImVec4 kDim{0.443F, 0.443F, 0.471F, 1.0F};
const ImVec4 kHeading{0.443F, 0.443F, 0.471F, 1.0F};
const ImVec4 kBright{0.95F, 0.96F, 0.97F, 1.0F};

bool sidebar_item(const char* label, bool selected, float width) {
    const ImVec2 size{width, kNavItemHeight};
    ImVec2 cursor = ImGui::GetCursorScreenPos();
    ImGui::PushID(label);
    ImGui::InvisibleButton("##nav", size);
    bool pressed = ImGui::IsItemActivated();
    bool hovered = ImGui::IsItemHovered();
    ImGui::PopID();

    auto* draw = ImGui::GetWindowDrawList();
    if (selected) {
        draw->AddRectFilled(cursor, ImVec2(cursor.x + size.x, cursor.y + size.y),
                            ImColor(255, 255, 255, 14), 6.0F);
        draw->AddRectFilled(ImVec2(cursor.x, cursor.y + 6),
                            ImVec2(cursor.x + 3, cursor.y + size.y - 6),
                            ImColor(kAccent.x, kAccent.y, kAccent.z, 1.0F), 1.5F);
    } else if (hovered) {
        draw->AddRectFilled(cursor, ImVec2(cursor.x + size.x, cursor.y + size.y),
                            ImColor(255, 255, 255, 8), 6.0F);
    }

    ImVec4 col = selected ? kBright : (hovered ? kBright : kDim);
    draw->AddText(ImVec2(cursor.x + 14, cursor.y + 8), ImColor(col), label);

    return pressed;
}

void draw_left_nav(stc::app::AppState& state) {
    // BeginChild with size.y = 0 auto-sizes to content, which leaves the sidebar bg covering only
    // the top portion of the window. Pass the parent's full available height instead.
    const float full_h = ImGui::GetContentRegionAvail().y;
    ImGui::BeginChild("##nav", ImVec2(kSidebarWidth, full_h), false,
                      ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    auto* draw = ImGui::GetWindowDrawList();
    ImVec2 win_pos = ImGui::GetWindowPos();
    ImVec2 win_size{kSidebarWidth, full_h};
    draw->AddRectFilled(win_pos, ImVec2(win_pos.x + win_size.x, win_pos.y + win_size.y),
                        ImColor(0.047F, 0.047F, 0.047F, 1.0F));
    draw->AddLine(ImVec2(win_pos.x + win_size.x - 1, win_pos.y),
                  ImVec2(win_pos.x + win_size.x - 1, win_pos.y + win_size.y),
                  ImColor(0.659F, 0.635F, 0.620F, 0.10F));

    ImGui::Dummy(ImVec2(0, kSidebarPaddingY));
    ImGui::Indent(kSidebarPaddingX);

    if (auto* tf = stc::ui::fonts::title()) {
        ImGui::PushFont(tf);
    }
    ImGui::TextUnformatted("Steam Tracer");
    ImGui::TextColored(kHeading, "Cleaner v0.1");
    if (stc::ui::fonts::title()) {
        ImGui::PopFont();
    }
    ImGui::Unindent(kSidebarPaddingX);

    ImGui::Dummy(ImVec2(0, 10));
    ImVec2 sep_a = ImGui::GetCursorScreenPos();
    sep_a.x = win_pos.x + kSidebarPaddingX;
    draw->AddLine(sep_a, ImVec2(win_pos.x + win_size.x - kSidebarPaddingX, sep_a.y),
                  ImColor(0.659F, 0.635F, 0.620F, 0.12F));

    auto draw_section = [&](const char* heading, std::initializer_list<NavSection::Item> items) {
        ImGui::Dummy(ImVec2(0, kSectionGap));
        ImGui::Indent(kSidebarPaddingX);
        ImGui::TextColored(kHeading, "%s", heading);
        ImGui::Unindent(kSidebarPaddingX);
        ImGui::Dummy(ImVec2(0, 4));
        for (const auto& item : items) {
            ImGui::SetCursorPosX(kSidebarPaddingX);
            if (sidebar_item(item.label, state.current_screen == item.screen,
                             kSidebarWidth - kSidebarPaddingX * 2.0F)) {
                state.current_screen = item.screen;
            }
            ImGui::Dummy(ImVec2(0, kNavItemSpacing));
        }
    };

    draw_section("Workspace", {
                                 {stc::app::Screen::Cleaner, "Cleaner", nullptr},
                                 {stc::app::Screen::Configs, "Configs", nullptr},
                                 {stc::app::Screen::Audit, "Audit", nullptr},
                             });
    draw_section("Manage", {
                              {stc::app::Screen::Backups, "Backups", nullptr},
                              {stc::app::Screen::Settings, "Settings", nullptr},
                          });

    ImGui::EndChild();
}

void draw_status_bar(stc::app::AppState& state) {
    auto* draw = ImGui::GetWindowDrawList();
    ImVec2 pos = ImGui::GetCursorScreenPos();

    char buf[512];
    if (state.install) {
        std::snprintf(buf, sizeof(buf),
                      "Steam   %s        Accounts   %zu        Libraries   %zu",
                      state.install->install_path.string().c_str(), state.accounts.size(),
                      state.libraries.size());
    } else {
        std::snprintf(buf, sizeof(buf), "Steam install not detected. Open Settings then Refresh.");
    }
    ImVec2 text_size = ImGui::CalcTextSize(buf);
    ImVec2 text_pos{pos.x + kContentPaddingX, pos.y + (kStatusBarHeight - text_size.y) * 0.5F};
    draw->AddText(text_pos, ImColor(kHeading.x, kHeading.y, kHeading.z, kHeading.w), buf);
}

void draw_content(stc::app::AppState& state) {
    ImGui::BeginGroup();

    // Footer margin and status bar live outside the scrollable region so screens don't reserve
    // for the footer in their own layout math.
    const float available = ImGui::GetContentRegionAvail().y;
    const float inner_h = available - kStatusBarHeight - kFooterMargin;

    ImGui::BeginChild("##content_inner", ImVec2(0, inner_h), false, ImGuiWindowFlags_None);

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,
                        ImVec2(ImGui::GetStyle().ItemSpacing.x, 10.0F));

    ImGui::Dummy(ImVec2(0, kContentPaddingY));
    ImGui::Indent(kContentPaddingX);
    ImGui::PushItemWidth(-kContentPaddingX);

    switch (state.current_screen) {
        case stc::app::Screen::Cleaner: stc::ui::screens::draw_cleaner_screen(state); break;
        case stc::app::Screen::Configs: stc::ui::screens::draw_configs_screen(state); break;
        case stc::app::Screen::Audit: stc::ui::screens::draw_audit_screen(state); break;
        case stc::app::Screen::Backups: stc::ui::screens::draw_backups_screen(state); break;
        case stc::app::Screen::Settings: stc::ui::screens::draw_settings_screen(state); break;
    }

    ImGui::PopItemWidth();
    ImGui::Unindent(kContentPaddingX);
    ImGui::PopStyleVar();
    ImGui::EndChild();

    draw_status_bar(state);

    ImGui::EndGroup();
}

}  // namespace

void draw_main_window(stc::app::AppState& state) {
    const ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(vp->WorkPos);
    ImGui::SetNextWindowSize(vp->WorkSize);

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                             ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
                             ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoDocking |
                             ImGuiWindowFlags_NoScrollbar;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    if (ImGui::Begin("##root", nullptr, flags)) {
        draw_left_nav(state);
        ImGui::SameLine(0, 0);
        draw_content(state);
    }
    ImGui::End();
    ImGui::PopStyleVar();
}

}  // namespace stc::ui
