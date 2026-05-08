#include "core/steam_paths.hpp"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <charconv>
#include <cstring>
#include <cwctype>
#include <system_error>

#include "core/vdf.hpp"
#include "platform/paths.hpp"
#include "platform/registry.hpp"

namespace stc::core::steam {
namespace {

namespace fs_std = std::filesystem;
namespace reg = stc::platform::reg;

// SteamID64 base for individual accounts: 0x0110000100000000.
constexpr std::uint64_t kSteamIdBase = 0x0110000100000000ULL;

std::optional<fs_std::path> read_install_path() {
    if (auto v = reg::read_string(HKEY_LOCAL_MACHINE, L"SOFTWARE\\WOW6432Node\\Valve\\Steam",
                                  L"InstallPath")) {
        return fs_std::path{*v};
    }
    if (auto v = reg::read_string(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Valve\\Steam", L"InstallPath")) {
        return fs_std::path{*v};
    }
    if (auto v = reg::read_string(HKEY_CURRENT_USER, L"Software\\Valve\\Steam", L"SteamPath")) {
        return fs_std::path{*v};
    }
    return std::nullopt;
}

}  // namespace

std::optional<InstallInfo> discover_install() {
    auto root = read_install_path();
    if (!root) {
        spdlog::warn("Steam install path not found in registry");
        return std::nullopt;
    }
    InstallInfo info;
    info.install_path = *root;
    info.config_dir = *root / "config";
    info.userdata_dir = *root / "userdata";
    info.appcache_dir = *root / "appcache";
    info.htmlcache_dir = stc::platform::local_appdata_dir() / "Steam" / "htmlcache";
    return info;
}

std::vector<fs_std::path> discover_libraries(const InstallInfo& install) {
    std::vector<fs_std::path> libs;
    libs.push_back(install.install_path / "steamapps");

    auto vdf_path = install.config_dir / "libraryfolders.vdf";
    auto doc = vdf::load(vdf_path);
    if (!doc) {
        spdlog::warn("libraryfolders.vdf missing or unreadable: {}", doc.error().message);
        return libs;
    }

    if (!doc->root || !doc->root->is_object()) {
        return libs;
    }

    // Modern format: numeric child keys "0", "1", ... each containing { "path" "..." }.
    for (const auto& [key, child] : doc->root->children()) {
        if (!child || !child->is_object()) {
            continue;
        }
        if (auto* p = child->find(L"path"); p && p->is_value()) {
            fs_std::path lib = fs_std::path{p->value()} / "steamapps";
            if (std::find(libs.begin(), libs.end(), lib) == libs.end()) {
                libs.push_back(std::move(lib));
            }
        }
    }
    return libs;
}

std::vector<AccountInfo> enumerate_accounts(const InstallInfo& install) {
    std::vector<AccountInfo> accounts;
    std::error_code ec;

    if (!fs_std::is_directory(install.userdata_dir, ec)) {
        return accounts;
    }

    for (auto it = fs_std::directory_iterator(install.userdata_dir, ec); !ec && it != fs_std::end(it);
         it.increment(ec)) {
        if (ec) {
            ec.clear();
            continue;
        }
        if (!it->is_directory(ec)) {
            continue;
        }
        auto name = it->path().filename().wstring();
        // userdata folder names are decimal account ids. Reject anything else.
        std::string narrow;
        narrow.reserve(name.size());
        bool valid = !name.empty();
        for (wchar_t ch : name) {
            if (ch < L'0' || ch > L'9') {
                valid = false;
                break;
            }
            narrow.push_back(static_cast<char>(ch));
        }
        if (!valid) {
            continue;
        }
        std::uint32_t account_id = 0;
        auto [_, errc] = std::from_chars(narrow.data(), narrow.data() + narrow.size(), account_id);
        if (errc != std::errc{} || account_id == 0) {
            continue;
        }
        AccountInfo a;
        a.account_id = account_id;
        a.steamid64 = account_id_to_steamid64(account_id);
        a.userdata_path = it->path();
        accounts.push_back(std::move(a));
    }

    // Merge in metadata from loginusers.vdf if present.
    auto doc = vdf::load(install.config_dir / "loginusers.vdf");
    if (doc && doc->root && doc->root->is_object()) {
        for (const auto& [steamid64, child] : doc->root->children()) {
            if (!child || !child->is_object()) {
                continue;
            }
            auto get = [&](std::wstring_view k) -> std::wstring {
                if (auto* n = child->find(k); n && n->is_value()) {
                    return n->value();
                }
                return {};
            };
            std::uint32_t aid = steamid64_to_account_id(steamid64);
            auto match = std::find_if(accounts.begin(), accounts.end(),
                                      [&](const AccountInfo& a) { return a.account_id == aid; });
            AccountInfo* dst = nullptr;
            if (match != accounts.end()) {
                dst = &*match;
            } else {
                AccountInfo n;
                n.steamid64 = steamid64;
                n.account_id = aid;
                accounts.push_back(std::move(n));
                dst = &accounts.back();
            }
            dst->account_name = get(L"AccountName");
            dst->persona_name = get(L"PersonaName");
            dst->most_recent = get(L"mostrecent") == L"1" || get(L"MostRecent") == L"1";
            dst->remember_password =
                get(L"RememberPassword") == L"1" || get(L"WantsOfflineMode") == L"1";
        }
    }

    return accounts;
}

std::wstring resolve_auto_login(const InstallInfo& install, std::wstring_view account_name) {
    if (account_name.empty()) {
        return {};
    }
    auto doc = vdf::load(install.config_dir / "loginusers.vdf");
    if (!doc || !doc->root) {
        return {};
    }
    for (const auto& [steamid64, child] : doc->root->children()) {
        if (!child || !child->is_object()) {
            continue;
        }
        if (auto* an = child->find(L"AccountName"); an && an->is_value() && an->value() == account_name) {
            return steamid64;
        }
    }
    return {};
}

std::uint32_t steamid64_to_account_id(std::wstring_view steamid64) {
    std::string narrow;
    narrow.reserve(steamid64.size());
    for (wchar_t ch : steamid64) {
        if (ch < L'0' || ch > L'9') {
            return 0;
        }
        narrow.push_back(static_cast<char>(ch));
    }
    std::uint64_t v = 0;
    auto [_, ec] = std::from_chars(narrow.data(), narrow.data() + narrow.size(), v);
    if (ec != std::errc{} || v < kSteamIdBase) {
        return 0;
    }
    return static_cast<std::uint32_t>(v - kSteamIdBase);
}

std::wstring account_id_to_steamid64(std::uint32_t account_id) {
    std::uint64_t v = kSteamIdBase + account_id;
    return std::to_wstring(v);
}

}  // namespace stc::core::steam
