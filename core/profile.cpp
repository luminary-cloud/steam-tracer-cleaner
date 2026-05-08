#include "core/profile.hpp"

#include <windows.h>

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

#include <fstream>
#include <system_error>
#include <vector>

namespace stc::core {
namespace {

std::wstring utf8_to_w(const std::string& s) {
    if (s.empty()) {
        return {};
    }
    int n = MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), nullptr, 0);
    std::wstring out(static_cast<std::size_t>(n), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), out.data(), n);
    return out;
}

std::string w_to_utf8(const std::wstring& s) {
    if (s.empty()) {
        return {};
    }
    int n = WideCharToMultiByte(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), nullptr, 0, nullptr,
                                nullptr);
    std::string out(static_cast<std::size_t>(n), '\0');
    WideCharToMultiByte(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), out.data(), n, nullptr,
                        nullptr);
    return out;
}

const std::vector<Profile>& builtins() {
    static const std::vector<Profile> profiles = [] {
        std::vector<Profile> p;
        p.push_back(Profile{
            L"Quick Clean",
            L"Caches, logs, dumps. Safe daily use.",
            {"steam.dumps", "steam.logs", "steam.appcache", "steam.appcache_stats",
             "steam.depotcache", "steam.shadercache", "steam.workshop_temp",
             "steam.steamapps_downloading", "steam.avatarcache", "steam.tenfoot_httpcache",
             "steam.overlayhtmlcache", "crash.local_appdata"},
            {},
            false});
        p.push_back(Profile{
            L"Account Reset",
            L"Quick Clean plus account residue. Surgically edits config.vdf and loginusers.vdf so "
            L"preserved accounts stay logged in. Removes ssfn files, autologin registry, htmlcache, "
            L"browser cookies, per-account caches.",
            {"steam.dumps", "steam.logs", "steam.appcache", "steam.appcache_stats",
             "steam.depotcache", "steam.shadercache", "steam.workshop_temp",
             "steam.steamapps_downloading", "steam.avatarcache", "steam.tenfoot_httpcache",
             "steam.overlayhtmlcache", "crash.local_appdata", "steam.htmlcache",
             "browser.steam_cookies", "steam.config_vdf", "steam.loginusers", "steam.ssfn",
             "steam.reg.autologin", "steam.reg.users", "steam.reg.activeprocess", "steam.reg.apps",
             "steam.reg.url_handler", "steam.remoteclients", "steam.userdata_inventory_cache",
             "steam.userdata_librarycache", "steam.userdata_sharedconfig"},
            {},
            false});
        p.push_back(Profile{
            L"Full Wipe",
            L"Account Reset plus userdata folders, controller bindings, screenshots, and non-Steam "
            L"shortcuts for non-preserved accounts. Destructive.",
            {"steam.dumps", "steam.logs", "steam.appcache", "steam.appcache_stats",
             "steam.depotcache", "steam.shadercache", "steam.workshop_temp",
             "steam.steamapps_downloading", "steam.avatarcache", "steam.tenfoot_httpcache",
             "steam.overlayhtmlcache", "crash.local_appdata", "steam.htmlcache",
             "browser.steam_cookies", "steam.config_vdf", "steam.loginusers", "steam.ssfn",
             "steam.reg.autologin", "steam.reg.users", "steam.reg.activeprocess", "steam.reg.apps",
             "steam.reg.url_handler", "steam.remoteclients", "steam.userdata",
             "steam.userdata_inventory_cache", "steam.userdata_librarycache",
             "steam.userdata_sharedconfig", "steam.userdata_shortcuts",
             "steam.userdata_screenshots", "steam.controller_configs"},
            {},
            true});
        p.push_back(Profile{
            L"Game Reset",
            L"Wipe selected AppIDs from each account's userdata. Other accounts and other games "
            L"are untouched. Pick AppIDs in the Cleaner screen.",
            {"steam.userdata"},
            {},  // populated by UI
            false});
        return p;
    }();
    return profiles;
}

}  // namespace

std::span<const Profile> built_in_profiles() {
    const auto& p = builtins();
    return {p.data(), p.size()};
}

std::optional<Profile> load_profile(const std::filesystem::path& json_path) {
    std::ifstream f(json_path);
    if (!f) {
        return std::nullopt;
    }
    nlohmann::json j;
    try {
        f >> j;
    } catch (const std::exception& e) {
        spdlog::warn("profile parse failed: {}", e.what());
        return std::nullopt;
    }

    Profile p;
    p.name = utf8_to_w(j.value("name", std::string{}));
    p.description = utf8_to_w(j.value("description", std::string{}));
    if (j.contains("target_ids")) {
        for (const auto& id : j.at("target_ids")) {
            p.target_ids.push_back(id.get<std::string>());
        }
    }
    if (j.contains("only_appids")) {
        for (const auto& id : j.at("only_appids")) {
            p.only_appids.push_back(id.get<std::uint32_t>());
        }
    }
    p.requires_confirmation = j.value("requires_confirmation", false);
    return p;
}

bool save_profile(const Profile& p, const std::filesystem::path& json_path) {
    nlohmann::json j;
    j["name"] = w_to_utf8(p.name);
    j["description"] = w_to_utf8(p.description);
    j["target_ids"] = p.target_ids;
    j["only_appids"] = p.only_appids;
    j["requires_confirmation"] = p.requires_confirmation;

    std::error_code ec;
    std::filesystem::create_directories(json_path.parent_path(), ec);
    std::ofstream f(json_path, std::ios::trunc);
    if (!f) {
        return false;
    }
    f << j.dump(4);
    return f.good();
}

}  // namespace stc::core
