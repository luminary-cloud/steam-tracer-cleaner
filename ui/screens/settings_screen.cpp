#include "ui/screens/settings_screen.hpp"

#include <imgui.h>

#include <algorithm>

#include "core/scheduled_clean.hpp"
#include "platform/paths.hpp"
#include "ui/util.hpp"

namespace stc::ui::screens {
namespace {

void draw_string_list(const char* label, const char* hint, std::vector<std::wstring>& list) {
    ImGui::SeparatorText(label);

    int remove_index = -1;
    for (std::size_t i = 0; i < list.size(); ++i) {
        std::string utf8 = stc::ui::to_utf8(list[i]);
        char buf[1024];
        std::snprintf(buf, sizeof(buf), "%s", utf8.c_str());
        std::string id = "##" + std::string{label} + std::to_string(i);
        ImGui::PushID(static_cast<int>(i));
        ImGui::SetNextItemWidth(-90);
        if (ImGui::InputText(id.c_str(), buf, sizeof(buf))) {
            list[i] = stc::ui::from_utf8(buf);
        }
        ImGui::SameLine();
        if (ImGui::SmallButton("Remove")) {
            remove_index = static_cast<int>(i);
        }
        ImGui::PopID();
    }
    if (remove_index >= 0) {
        list.erase(list.begin() + remove_index);
    }
    if (ImGui::Button(("Add##" + std::string{label}).c_str())) {
        list.emplace_back();
    }
    if (hint) {
        ImGui::SameLine();
        ImGui::TextDisabled("%s", hint);
    }
}

}  // namespace

void draw_settings_screen(stc::app::AppState& state) {
    ImGui::SeparatorText("Steam");
    if (state.install) {
        ImGui::Text("Install: %s", state.install->install_path.string().c_str());
        ImGui::Text("Userdata: %s", state.install->userdata_dir.string().c_str());
        ImGui::Text("Libraries: %zu", state.libraries.size());
        ImGui::Text("Accounts: %zu", state.accounts.size());
    } else {
        ImGui::TextDisabled("Not detected");
    }
    if (ImGui::Button("Refresh")) {
        state.refresh_steam();
    }
    stc::ui::hover_tooltip("Re-read the Steam install path from the registry, then re-enumerate "
                           "library drives and accounts.");

    ImGui::Spacing();
    ImGui::SeparatorText("Mode");
    ImGui::Text("Portable mode: %s", state.portable_mode ? "yes" : "no");
    ImGui::TextDisabled("Drop a 'portable.flag' file next to the .exe to enable. Restart required.");
    ImGui::Text("Config dir: %s", state.config_dir.string().c_str());
    ImGui::Text("Data dir:   %s", state.data_dir.string().c_str());

    ImGui::Spacing();
    ImGui::SeparatorText("Behavior");
    ImGui::Checkbox("Back up by default before destructive actions", &state.backup_by_default);
    stc::ui::hover_tooltip("When enabled, the Cleaner page's Clean button still performs a backup "
                           "first. Disable to skip backups by default and free disk faster.");
    ImGui::Checkbox("Confirm before running the Full Wipe profile", &state.confirm_full_wipe);
    stc::ui::hover_tooltip("Show a confirmation popup before the Full Wipe profile runs. Full Wipe "
                           "removes userdata folders for non-preserved accounts.");

    ImGui::SetNextItemWidth(140);
    ImGui::InputInt("Keep last N backups", &state.backup_keep_count);
    if (state.backup_keep_count < 0) {
        state.backup_keep_count = 0;
    }
    stc::ui::hover_tooltip("After each Backup-and-clean run, older backups under "
                           "%LOCALAPPDATA%/steam-tracer-cleaner/backups are removed so at most "
                           "this many remain. 0 means keep all.");

    ImGui::Spacing();
    ImGui::SeparatorText("Scheduled cleanup");
    static bool scheduled_state_known = false;
    static bool scheduled_enabled = false;
    if (!scheduled_state_known) {
        scheduled_enabled = stc::core::scheduled::query().registered;
        scheduled_state_known = true;
    }
    if (ImGui::Checkbox("Run Quick Clean automatically when this user logs on",
                        &scheduled_enabled)) {
        if (scheduled_enabled) {
            scheduled_enabled = stc::core::scheduled::register_logon_task(stc::platform::exe_path());
        } else {
            stc::core::scheduled::unregister();
            scheduled_enabled = false;
        }
    }
    stc::ui::hover_tooltip("Registers a Windows Task Scheduler task that launches "
                           "steam-tracer-cleaner.exe --scheduled at user logon and runs the Quick "
                           "Clean profile silently. Toggling off unregisters the task.");
    ImGui::TextDisabled("Registers a Windows Task Scheduler task that runs the Quick Clean profile "
                        "in the background.");

    ImGui::Spacing();
    ImGui::SeparatorText("Ignore list");
    ImGui::Checkbox("Preserve all ssfn (Steam Guard sentry) files",
                    &state.ignore_list.preserve_all_ssfn);
    stc::ui::hover_tooltip(
        "Skip every ssfn* file. Steam uses these to remember 2FA-trusted machines, so preserving "
        "them keeps logged-in accounts from being prompted for Steam Guard codes after a clean. "
        "ssfn files cannot be filtered per-account, only globally.");

    draw_string_list("Preserved account ids (SteamID64)", nullptr,
                     state.ignore_list.preserved_account_ids);
    stc::ui::hover_tooltip(
        "SteamID64s that survive every cleanup. The cleaner preserves the matching "
        "loginusers.vdf entry, the userdata\\<account_id>\\ folder, the "
        "HKCU\\Software\\Valve\\Steam\\Users\\<id> registry subtree, and that account's "
        "controller_configs directory. When AutoLoginUser currently points at a preserved "
        "account, AutoLoginUser, RememberPassword, and LastGameNameUsed are also kept. Whenever "
        "any account is preserved, remoteclients.vdf and coplay_* files are left alone too "
        "(their per-account contents can't be split cleanly). ssfn files are not per-account; "
        "use the global toggle above to keep them.");

    draw_string_list("Preserved paths (prefix match)", "case-insensitive",
                     state.ignore_list.preserved_paths);
    stc::ui::hover_tooltip(
        "Case-insensitive path prefixes. Any file or directory under these paths is left alone "
        "during cleanup. Useful for keeping a custom Steam config that lives next to the cache "
        "folders the cleaner targets.");

    draw_string_list("Preserved registry values",
                     "Format: HKCU\\Software\\Valve\\Steam\\Users\\<id> or KEY::VALUE",
                     state.ignore_list.preserved_registry_values);
    stc::ui::hover_tooltip(
        "Registry paths or KEY::VALUE pairs to skip. Plain path is a prefix match (preserves the "
        "key and everything below it). KEY::VALUE form preserves a single named value under that "
        "key.");

    ImGui::Spacing();
    if (ImGui::Button("Save settings", ImVec2(160, 32))) {
        state.save_settings();
    }
    stc::ui::hover_tooltip("Persist the ignore list to ignore.json under the config directory.");
}

}  // namespace stc::ui::screens
