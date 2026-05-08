#include "app/app_state.hpp"

#include <spdlog/spdlog.h>

#include <fstream>
#include <system_error>

#include "platform/paths.hpp"

namespace stc::app {
namespace {

bool portable_flag_present(const std::filesystem::path& exe_dir) {
    std::error_code ec;
    return std::filesystem::exists(exe_dir / "portable.flag", ec);
}

}  // namespace

void AppState::initialize() {
    auto exe_dir = stc::platform::exe_directory();
    portable_mode = portable_flag_present(exe_dir);

    if (portable_mode) {
        config_dir = exe_dir / "data";
        data_dir = exe_dir / "data";
    } else {
        config_dir = stc::platform::appdata_dir() / "steam-tracer-cleaner";
        data_dir = stc::platform::local_appdata_dir() / "steam-tracer-cleaner";
    }
    backups_dir = data_dir / "backups";

    std::error_code ec;
    std::filesystem::create_directories(config_dir, ec);
    std::filesystem::create_directories(data_dir, ec);
    std::filesystem::create_directories(backups_dir, ec);

    load_settings();
    refresh_steam();

    profiles.assign(stc::core::built_in_profiles().begin(), stc::core::built_in_profiles().end());

    if (!profiles.empty()) {
        const auto& first = profiles.front();
        selected_target_ids.insert(first.target_ids.begin(), first.target_ids.end());
    }
}

void AppState::refresh_steam() {
    install = stc::core::steam::discover_install();
    if (install) {
        libraries = stc::core::steam::discover_libraries(*install);
        accounts = stc::core::steam::enumerate_accounts(*install);
        spdlog::info("Steam found at {}, {} libraries, {} accounts",
                     install->install_path.string(), libraries.size(), accounts.size());
    } else {
        libraries.clear();
        accounts.clear();
        spdlog::warn("Steam install not found");
    }
}

void AppState::load_settings() {
    auto ignore = stc::core::load_ignore_list(config_dir / "ignore.json");
    if (ignore) {
        ignore_list = std::move(*ignore);
    } else {
        ignore_list = stc::core::default_ignore_list();
    }
}

void AppState::save_settings() {
    stc::core::save_ignore_list(ignore_list, config_dir / "ignore.json");
}

}  // namespace stc::app
