#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

#include "core/steam_paths.hpp"

namespace stc::core::audit {

struct AccountSummary {
    std::wstring steamid64;
    std::wstring persona_name;
    std::wstring account_name;
    std::uintmax_t userdata_bytes = 0;
    bool has_localconfig = false;
    bool has_loginusers_entry = false;
    bool ssfn_present = false;
};

struct BrowserSummary {
    std::wstring browser;            // "Chrome", "Edge", "Firefox", "Brave"
    std::wstring profile;
    std::filesystem::path cookies_db;
    bool reachable = false;          // file exists and process not holding it
    int steam_cookies = 0;           // -1 = not yet counted
};

struct HwidSnapshot {
    std::wstring machine_guid;       // HKLM\SOFTWARE\Microsoft\Cryptography\MachineGuid
    std::wstring machine_id;         // HKLM\SOFTWARE\Microsoft\SQMClient\MachineId
    std::wstring computer_name;
};

struct CrashDumpSummary {
    std::filesystem::path dir;
    std::uintmax_t count = 0;
    std::uintmax_t total_bytes = 0;
};

struct Report {
    std::vector<AccountSummary> accounts;
    std::vector<BrowserSummary> browsers;
    HwidSnapshot hwid;
    std::vector<CrashDumpSummary> crash_dumps;
};

Report build_report(const stc::core::steam::InstallInfo& install,
                    const std::vector<stc::core::steam::AccountInfo>& accounts);

// Renders `r` into a plain-text string suitable for export.
std::string render_text(const Report& r);

}  // namespace stc::core::audit
