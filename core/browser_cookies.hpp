#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace stc::core::browser {

enum class Schema { Chromium, Firefox };

struct CookieDb {
    std::wstring browser_label;     // "Chrome", "Edge", ...
    std::wstring profile_label;
    std::filesystem::path db_path;
    Schema schema;
};

// Discovers cookie DBs across known browsers under %LOCALAPPDATA% and %APPDATA%.
std::vector<CookieDb> discover_cookie_dbs();

// Deletes Steam-related cookies from `db_path`. Returns the number of rows removed, or -1 on
// failure. The DB is locked while the browser is running; the caller must close the browser first.
int clear_steam_cookies(const std::filesystem::path& db_path, Schema schema);

}  // namespace stc::core::browser
