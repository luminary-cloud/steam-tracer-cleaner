#include "core/browser_cookies.hpp"

#include <sqlite3.h>
#include <spdlog/spdlog.h>

#include <array>
#include <system_error>

#include "platform/paths.hpp"

namespace stc::core::browser {
namespace {

namespace fs_std = std::filesystem;

// Steam-owned domains we want to scrub. The leading dot makes Chromium's host_key suffix-match work.
constexpr std::array<const char*, 6> kSteamHosts = {{
    "steamcommunity.com",
    "store.steampowered.com",
    "help.steampowered.com",
    "login.steampowered.com",
    "checkout.steampowered.com",
    "steampowered.com",
}};

bool find_first_chromium_db(const fs_std::path& profile_dir, fs_std::path& out) {
    std::error_code ec;
    auto network = profile_dir / "Network" / "Cookies";
    if (fs_std::exists(network, ec)) {
        out = network;
        return true;
    }
    auto legacy = profile_dir / "Cookies";
    if (fs_std::exists(legacy, ec)) {
        out = legacy;
        return true;
    }
    return false;
}

void scan_chromium_root(std::vector<CookieDb>& out, std::wstring_view label,
                        const fs_std::path& root) {
    std::error_code ec;
    if (!fs_std::is_directory(root, ec)) {
        return;
    }
    for (auto it = fs_std::directory_iterator(root, ec); !ec && it != fs_std::end(it);
         it.increment(ec)) {
        if (!it->is_directory(ec)) {
            continue;
        }
        fs_std::path db;
        if (!find_first_chromium_db(it->path(), db)) {
            continue;
        }
        CookieDb e;
        e.browser_label = label;
        e.profile_label = it->path().filename().wstring();
        e.db_path = db;
        e.schema = Schema::Chromium;
        out.push_back(std::move(e));
    }
}

}  // namespace

std::vector<CookieDb> discover_cookie_dbs() {
    std::vector<CookieDb> out;
    auto local = stc::platform::local_appdata_dir();
    auto roaming = stc::platform::appdata_dir();

    scan_chromium_root(out, L"Chrome", local / "Google" / "Chrome" / "User Data");
    scan_chromium_root(out, L"Edge", local / "Microsoft" / "Edge" / "User Data");
    scan_chromium_root(out, L"Brave", local / "BraveSoftware" / "Brave-Browser" / "User Data");
    scan_chromium_root(out, L"Vivaldi", local / "Vivaldi" / "User Data");

    // Firefox profiles each ship a cookies.sqlite directly.
    std::error_code ec;
    auto ff_root = roaming / "Mozilla" / "Firefox" / "Profiles";
    if (fs_std::is_directory(ff_root, ec)) {
        for (auto it = fs_std::directory_iterator(ff_root, ec); !ec && it != fs_std::end(it);
             it.increment(ec)) {
            auto db = it->path() / "cookies.sqlite";
            if (fs_std::exists(db, ec)) {
                CookieDb e;
                e.browser_label = L"Firefox";
                e.profile_label = it->path().filename().wstring();
                e.db_path = db;
                e.schema = Schema::Firefox;
                out.push_back(std::move(e));
            }
        }
    }
    return out;
}

int clear_steam_cookies(const fs_std::path& db_path, Schema schema) {
    sqlite3* db = nullptr;
    int rc = sqlite3_open_v2(db_path.string().c_str(), &db,
                             SQLITE_OPEN_READWRITE | SQLITE_OPEN_FULLMUTEX, nullptr);
    if (rc != SQLITE_OK) {
        spdlog::warn("sqlite3_open_v2({}) failed: {}", db_path.string(),
                     db ? sqlite3_errmsg(db) : "unknown");
        if (db) {
            sqlite3_close(db);
        }
        return -1;
    }

    sqlite3_busy_timeout(db, 1500);

    const char* sql = nullptr;
    if (schema == Schema::Chromium) {
        sql = "DELETE FROM cookies WHERE "
              "host_key LIKE '%steamcommunity.com' OR host_key LIKE '%steampowered.com'";
    } else {
        sql = "DELETE FROM moz_cookies WHERE "
              "host LIKE '%steamcommunity.com' OR host LIKE '%steampowered.com'";
    }

    char* err = nullptr;
    rc = sqlite3_exec(db, sql, nullptr, nullptr, &err);
    int removed = -1;
    if (rc == SQLITE_OK) {
        removed = sqlite3_changes(db);
    } else {
        spdlog::warn("sqlite3_exec failed for {}: {}", db_path.string(), err ? err : "?");
        if (err) {
            sqlite3_free(err);
        }
    }
    sqlite3_close(db);
    return removed;
}

}  // namespace stc::core::browser
