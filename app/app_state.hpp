#pragma once

#include <filesystem>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include "core/cleaner.hpp"
#include "core/dry_run.hpp"
#include "core/ignore_list.hpp"
#include "core/profile.hpp"
#include "core/steam_paths.hpp"

namespace stc::app {

enum class Screen { Cleaner, Configs, Audit, Backups, Settings };

struct AppState {
    std::filesystem::path config_dir;     // ignore.json, profiles/
    std::filesystem::path data_dir;       // logs/, backups/
    std::filesystem::path backups_dir;
    bool portable_mode = false;

    std::optional<stc::core::steam::InstallInfo> install;
    std::vector<stc::core::steam::AccountInfo> accounts;
    std::vector<std::filesystem::path> libraries;

    stc::core::IgnoreList ignore_list;
    std::vector<stc::core::Profile> profiles;

    Screen current_screen = Screen::Cleaner;

    // Cleaner screen selection state
    int selected_profile_index = 0;
    std::set<std::string> selected_target_ids;
    std::set<std::uint32_t> selected_appids;        // Game Reset profile
    std::optional<stc::core::Plan> last_plan;
    std::optional<stc::core::CleanResult> last_result;

    // Configs screen selection state
    std::filesystem::path autoexec_source;
    std::uint32_t autoexec_target_appid = 730;       // CS2 default
    std::filesystem::path video_config_source;
    std::set<std::uint32_t> video_config_targets;

    // Settings
    bool backup_by_default = true;
    bool confirm_full_wipe = true;
    int backup_keep_count = 10;       // auto-prune older backups; 0 means keep all

    void initialize();
    void refresh_steam();
    void load_settings();
    void save_settings();
};

}  // namespace stc::app
