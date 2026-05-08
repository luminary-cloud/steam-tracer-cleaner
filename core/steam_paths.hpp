#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace stc::core::steam {

struct InstallInfo {
    std::filesystem::path install_path;   // C:/Program Files (x86)/Steam
    std::filesystem::path config_dir;     // <install>/config
    std::filesystem::path userdata_dir;   // <install>/userdata
    std::filesystem::path appcache_dir;   // <install>/appcache
    std::filesystem::path htmlcache_dir;  // %LOCALAPPDATA%/Steam/htmlcache
};

std::optional<InstallInfo> discover_install();

// Reads <install>/config/libraryfolders.vdf and returns every library root, including the primary
// install. Falls back to {install_path/steamapps} if the file is missing or malformed.
std::vector<std::filesystem::path> discover_libraries(const InstallInfo& install);

struct AccountInfo {
    std::wstring steamid64;          // "76561198..."
    std::uint32_t account_id = 0;    // SteamID3 lower 32 bits, used for userdata folder names
    std::wstring account_name;       // login name from loginusers.vdf
    std::wstring persona_name;       // display name from loginusers.vdf
    bool most_recent = false;        // mostrecent flag in loginusers.vdf
    bool remember_password = false;
    std::filesystem::path userdata_path;  // <install>/userdata/<account_id>
};

// Enumerates accounts from <install>/userdata/* and merges metadata from loginusers.vdf.
std::vector<AccountInfo> enumerate_accounts(const InstallInfo& install);

// Resolves AutoLoginUser (account_name) to a SteamID64 via loginusers.vdf. Returns empty if not
// found.
std::wstring resolve_auto_login(const InstallInfo& install, std::wstring_view account_name);

// Converts a SteamID64 to its 32-bit account id (the userdata folder name).
std::uint32_t steamid64_to_account_id(std::wstring_view steamid64);

// Converts the account id (32-bit) back to a SteamID64 string.
std::wstring account_id_to_steamid64(std::uint32_t account_id);

}  // namespace stc::core::steam
