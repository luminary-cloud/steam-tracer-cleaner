#include "ui/screens/configs_screen.hpp"

#include <windows.h>

#include <commdlg.h>
#include <imgui.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <array>

#include "core/appid_map.hpp"
#include "core/autoexec.hpp"
#include "platform/process.hpp"
#include "ui/util.hpp"
#include "ui/widgets/account_picker.hpp"

namespace stc::ui::screens {
namespace {

std::filesystem::path open_file_dialog(HWND owner, const wchar_t* filter, const wchar_t* title) {
    wchar_t path[MAX_PATH] = L"";
    OPENFILENAMEW ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = owner;
    ofn.lpstrFilter = filter;
    ofn.lpstrFile = path;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrTitle = title;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
    if (!GetOpenFileNameW(&ofn)) {
        return {};
    }
    return std::filesystem::path{path};
}

void draw_autoexec_tab(stc::app::AppState& state) {
    ImGui::TextWrapped(
        "Picks a .cfg file and copies it to the game's cfg directory. Source 1 games auto-execute "
        "autoexec.cfg at startup. Source 2 games (CS2, Dota 2) require '+exec autoexec.cfg' in "
        "launch options.");
    ImGui::Spacing();

    ImGui::Text("Game");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(260);
    auto games = stc::core::appid::all_games();
    int current_idx = 0;
    std::vector<std::string> labels;
    labels.reserve(games.size());
    for (std::size_t i = 0; i < games.size(); ++i) {
        labels.push_back(stc::ui::to_utf8(games[i].display_name));
        if (games[i].appid == state.autoexec_target_appid) {
            current_idx = static_cast<int>(i);
        }
    }
    std::vector<const char*> labels_c;
    labels_c.reserve(labels.size());
    for (auto& s : labels) {
        labels_c.push_back(s.c_str());
    }
    if (ImGui::Combo("##autoexec_game", &current_idx, labels_c.data(),
                     static_cast<int>(labels_c.size()))) {
        state.autoexec_target_appid = games[current_idx].appid;
    }
    stc::ui::hover_tooltip("Target game. The autoexec.cfg goes to this game's cfg directory across "
                           "every Steam library on the machine.");
    ImGui::Spacing();

    ImGui::Text("Source .cfg:");
    ImGui::SameLine();
    if (state.autoexec_source.empty()) {
        ImGui::TextDisabled("(no file picked)");
    } else {
        ImGui::TextUnformatted(state.autoexec_source.string().c_str());
    }
    if (ImGui::Button("Pick file...##autoexec")) {
        auto p = open_file_dialog(nullptr, L"Config files (*.cfg)\0*.cfg\0All files (*.*)\0*.*\0\0",
                                  L"Pick autoexec.cfg");
        if (!p.empty()) {
            state.autoexec_source = std::move(p);
        }
    }
    stc::ui::hover_tooltip("Browse for a .cfg file to copy as the game's autoexec.cfg.");

    ImGui::Spacing();
    bool can_install = !state.autoexec_source.empty() && !state.libraries.empty();
    if (!can_install) {
        ImGui::BeginDisabled();
    }
    if (ImGui::Button("Install autoexec", ImVec2(180, 32))) {
        auto r = stc::core::autoexec::install_autoexec(state.libraries, state.autoexec_target_appid,
                                                       state.autoexec_source);
        spdlog::info("Autoexec install: ok={} target={}", r.ok, r.target_path.string());
        if (r.ok) {
            ImGui::OpenPopup("Autoexec installed");
        } else {
            ImGui::OpenPopup("Autoexec failed");
        }
    }
    stc::ui::hover_tooltip(
        "Copy the picked .cfg to the selected game's cfg directory. Existing autoexec.cfg is "
        "renamed to autoexec.cfg.bak.<timestamp> first.");
    if (!can_install) {
        ImGui::EndDisabled();
    }

    if (ImGui::BeginPopupModal("Autoexec installed", nullptr,
                               ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings)) {
        ImGui::TextWrapped("autoexec.cfg copied. If this is a Source 2 game, remember to add "
                           "'+exec autoexec.cfg' to launch options.");
        if (ImGui::Button("OK")) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
    if (ImGui::BeginPopupModal("Autoexec failed", nullptr,
                               ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings)) {
        ImGui::TextWrapped(
            "Could not find an installed copy of the selected game across known Steam libraries.");
        if (ImGui::Button("OK")) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void draw_video_config_tab(stc::app::AppState& state) {
    ImGui::TextWrapped(
        "Picks a cs2_video.txt and copies it to userdata\\<account>\\730\\local\\cfg\\cs2_video.txt "
        "for each selected account. Existing files are renamed to .bak before overwrite.");
    ImGui::Spacing();

    ImGui::Text("Source cs2_video.txt:");
    ImGui::SameLine();
    if (state.video_config_source.empty()) {
        ImGui::TextDisabled("(no file picked)");
    } else {
        ImGui::TextUnformatted(state.video_config_source.string().c_str());
    }
    if (ImGui::Button("Pick file...##video")) {
        auto p = open_file_dialog(
            nullptr, L"Steam config (*.txt)\0*.txt\0All files (*.*)\0*.*\0\0", L"Pick cs2_video.txt");
        if (!p.empty()) {
            state.video_config_source = std::move(p);
        }
    }
    stc::ui::hover_tooltip("Browse for a cs2_video.txt to deploy to selected accounts.");

    ImGui::Spacing();
    ImGui::SeparatorText("Target accounts");
    stc::ui::widgets::account_picker(state.accounts, state.video_config_targets);

    ImGui::Spacing();
    bool can_install =
        !state.video_config_source.empty() && !state.video_config_targets.empty() && state.install;
    if (stc::platform::proc::is_running(L"steam.exe")) {
        ImGui::TextColored(ImVec4(1.0F, 0.7F, 0.2F, 1.0F),
                           "Steam is running. Close Steam before installing video configs.");
        can_install = false;
    }
    if (!can_install) {
        ImGui::BeginDisabled();
    }
    bool install_pressed = ImGui::Button("Install to selected accounts", ImVec2(260, 32));
    stc::ui::hover_tooltip("For each ticked account, copy the picked file to "
                           "userdata/<id>/730/local/cfg/cs2_video.txt. Existing files are renamed "
                           "to .bak.<timestamp> first.");
    if (install_pressed) {
        std::vector<std::uint32_t> ids(state.video_config_targets.begin(),
                                       state.video_config_targets.end());
        auto result = stc::core::autoexec::install_video_config(*state.install, state.accounts, ids,
                                                                730, state.video_config_source);
        spdlog::info("Video config: ok={} attempted={} succeeded={}", result.ok, result.per_account.size(),
                     std::count_if(result.per_account.begin(), result.per_account.end(),
                                   [](const auto& r) { return r.ok; }));
        ImGui::OpenPopup("Video config result");
    }
    if (!can_install) {
        ImGui::EndDisabled();
    }

    if (ImGui::BeginPopupModal("Video config result", nullptr,
                               ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings)) {
        ImGui::Text("Installed cs2_video.txt to %zu account(s).", state.video_config_targets.size());
        if (ImGui::Button("OK")) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

}  // namespace

void draw_configs_screen(stc::app::AppState& state) {
    if (!state.install) {
        ImGui::TextWrapped("Steam install not detected. Open the Settings tab and click Refresh.");
        return;
    }

    if (ImGui::BeginTabBar("##configs_tabs")) {
        if (ImGui::BeginTabItem("Autoexec")) {
            draw_autoexec_tab(state);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Video config")) {
            draw_video_config_tab(state);
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
}

}  // namespace stc::ui::screens
