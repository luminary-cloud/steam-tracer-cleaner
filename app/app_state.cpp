#include "app/app_state.hpp"

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

#include <fstream>
#include <system_error>

#include "core/version.hpp"
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
    configs_dir = data_dir / "configs";

    std::error_code ec;
    std::filesystem::create_directories(config_dir, ec);
    std::filesystem::create_directories(data_dir, ec);
    std::filesystem::create_directories(backups_dir, ec);
    std::filesystem::create_directories(configs_dir / "autoexec", ec);
    std::filesystem::create_directories(configs_dir / "video", ec);

    load_settings();
    refresh_steam();
    refresh_config_library();

    profiles.assign(stc::core::built_in_profiles().begin(), stc::core::built_in_profiles().end());

    if (!profiles.empty()) {
        const auto& first = profiles.front();
        selected_target_ids.insert(first.target_ids.begin(), first.target_ids.end());
    }

    start_update_check();
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

void AppState::refresh_config_library() {
    using CK = stc::core::config_library::ConfigKind;
    autoexec_library = stc::core::config_library::list_configs(configs_dir, CK::Autoexec);
    video_library = stc::core::config_library::list_configs(configs_dir, CK::Video);
}

void AppState::load_settings() {
    auto ignore = stc::core::load_ignore_list(config_dir / "ignore.json");
    if (ignore) {
        ignore_list = std::move(*ignore);
    } else {
        ignore_list = stc::core::default_ignore_list();
    }
    load_app_settings();
}

void AppState::save_settings() {
    stc::core::save_ignore_list(ignore_list, config_dir / "ignore.json");
    save_app_settings();
}

void AppState::load_app_settings() {
    std::ifstream f(config_dir / "settings.json");
    if (!f) {
        return;
    }
    try {
        nlohmann::json j;
        f >> j;
        check_updates_on_launch = j.value("check_updates_on_launch", true);
        version_check_skip_until = j.value("version_check_skip_until", std::string{});
        backup_by_default = j.value("backup_by_default", backup_by_default);
        confirm_full_wipe = j.value("confirm_full_wipe", confirm_full_wipe);
        backup_keep_count = j.value("backup_keep_count", backup_keep_count);
    } catch (const std::exception& e) {
        spdlog::warn("settings.json parse failed: {}", e.what());
    }
}

void AppState::save_app_settings() {
    nlohmann::json j;
    j["check_updates_on_launch"] = check_updates_on_launch;
    j["version_check_skip_until"] = version_check_skip_until;
    j["backup_by_default"] = backup_by_default;
    j["confirm_full_wipe"] = confirm_full_wipe;
    j["backup_keep_count"] = backup_keep_count;

    std::error_code ec;
    std::filesystem::create_directories(config_dir, ec);
    std::ofstream f(config_dir / "settings.json", std::ios::trunc);
    if (!f) {
        spdlog::warn("settings.json: open for write failed");
        return;
    }
    f << j.dump(4);
}

void AppState::start_update_check() {
    if (!check_updates_on_launch) {
        return;
    }
    update_thread = std::jthread([this](std::stop_token st) {
        if (st.stop_requested()) {
            return;
        }
        auto r = stc::core::update_check::fetch_latest_release(
            "luminary-cloud/steam-tracer-cleaner", stc::core::kAppVersion);
        if (!r || !r->newer_than_current || st.stop_requested()) {
            return;
        }
        std::lock_guard lk(update_mutex);
        update_result = std::move(r);
    });
}

}  // namespace stc::app
