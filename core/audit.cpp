// Audit module.
//
// The audit screen is read-only. It walks the same artifact catalog the cleaner uses, but it
// only reports what it finds and never mutates the disk or the registry. The HWID block reads
// MachineGuid / MachineId / computer name straight out of HKLM and HKEY_CURRENT_USER for
// display, again with no writes. The reason for this hard split is two-fold:
//
//   1. Users who don't trust the cleaner yet should be able to verify what the tool would touch
//      without taking the risk of actually touching anything.
//   2. Anti-cheat / Valve / forensic readers should be able to look at this code and confirm it
//      cannot be flipped into a "modify" path through some setting. The whole module returns
//      structs by value; nothing here calls anything that writes.
//
// If you ever feel the urge to add a "fix this" button to the audit screen, don't. Add it to a
// separate cleaner profile instead so the audit-as-trust-anchor property survives.

#include "core/audit.hpp"

#include <windows.h>

#include <spdlog/spdlog.h>

#include <array>
#include <sstream>
#include <system_error>

#include "platform/fs.hpp"
#include "platform/paths.hpp"
#include "platform/registry.hpp"

namespace stc::core::audit {
namespace {

namespace fs_std = std::filesystem;

std::string utf8(const std::wstring& s) {
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

}  // namespace

Report build_report(const stc::core::steam::InstallInfo& install,
                    const std::vector<stc::core::steam::AccountInfo>& accounts) {
    Report r;

    for (const auto& a : accounts) {
        AccountSummary s;
        s.steamid64 = a.steamid64;
        s.persona_name = a.persona_name;
        s.account_name = a.account_name;
        s.userdata_bytes = stc::platform::fsx::size_recursive(a.userdata_path);
        s.has_localconfig = stc::platform::fsx::exists(a.userdata_path / "config" / "localconfig.vdf");
        s.has_loginusers_entry = !a.account_name.empty();
        r.accounts.push_back(std::move(s));
    }

    auto local = stc::platform::local_appdata_dir();
    auto roaming = stc::platform::appdata_dir();
    struct ChromiumProfile {
        const wchar_t* browser;
        fs_std::path base;
    };
    std::array<ChromiumProfile, 5> chromium = {{
        {L"Chrome", local / "Google" / "Chrome" / "User Data"},
        {L"Edge", local / "Microsoft" / "Edge" / "User Data"},
        {L"Brave", local / "BraveSoftware" / "Brave-Browser" / "User Data"},
        {L"Vivaldi", local / "Vivaldi" / "User Data"},
        {L"Opera", roaming / "Opera Software" / "Opera Stable"},
    }};
    for (const auto& browser : chromium) {
        std::error_code ec;
        if (!fs_std::is_directory(browser.base, ec)) {
            continue;
        }
        for (auto it = fs_std::directory_iterator(browser.base, ec); !ec && it != fs_std::end(it);
             it.increment(ec)) {
            if (!it->is_directory(ec)) {
                continue;
            }
            auto cookies = it->path() / "Network" / "Cookies";
            if (!fs_std::exists(cookies, ec)) {
                cookies = it->path() / "Cookies";
            }
            if (fs_std::exists(cookies, ec)) {
                BrowserSummary b;
                b.browser = browser.browser;
                b.profile = it->path().filename().wstring();
                b.cookies_db = cookies;
                b.reachable = true;
                b.steam_cookies = -1;
                r.browsers.push_back(std::move(b));
            }
        }
    }

    auto firefox_profiles = roaming / "Mozilla" / "Firefox" / "Profiles";
    std::error_code ec;
    if (fs_std::is_directory(firefox_profiles, ec)) {
        for (auto it = fs_std::directory_iterator(firefox_profiles, ec);
             !ec && it != fs_std::end(it); it.increment(ec)) {
            auto cookies = it->path() / "cookies.sqlite";
            if (fs_std::exists(cookies, ec)) {
                BrowserSummary b;
                b.browser = L"Firefox";
                b.profile = it->path().filename().wstring();
                b.cookies_db = cookies;
                b.reachable = true;
                b.steam_cookies = -1;
                r.browsers.push_back(std::move(b));
            }
        }
    }

    r.hwid.machine_guid = stc::platform::reg::read_string_or_empty(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Cryptography",
                                        L"MachineGuid");
    r.hwid.machine_id = stc::platform::reg::read_string_or_empty(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\SQMClient",
                                      L"MachineId");

    DWORD cn_size = 256;
    std::wstring cn(cn_size, L'\0');
    if (GetComputerNameW(cn.data(), &cn_size)) {
        cn.resize(cn_size);
        r.hwid.computer_name = std::move(cn);
    }

    std::array<fs_std::path, 3> dump_dirs = {
        stc::platform::windows_dir() / "Minidump",
        stc::platform::local_appdata_dir() / "CrashDumps",
        install.install_path / "dumps",
    };
    for (const auto& d : dump_dirs) {
        if (!fs_std::is_directory(d, ec)) {
            continue;
        }
        CrashDumpSummary c;
        c.dir = d;
        for (auto it = fs_std::directory_iterator(d, ec); !ec && it != fs_std::end(it);
             it.increment(ec)) {
            if (!it->is_regular_file(ec)) {
                continue;
            }
            auto ext = it->path().extension();
            if (ext == L".dmp" || ext == L".mdmp") {
                c.count += 1;
                std::error_code ec2;
                c.total_bytes += it->file_size(ec2);
            }
        }
        r.crash_dumps.push_back(std::move(c));
    }

    return r;
}

std::string render_text(const Report& r) {
    std::ostringstream s;
    s << "=== Steam Tracer Cleaner: audit report ===\n\n";

    s << "[HWID snapshot]\n";
    s << "  Computer name : " << utf8(r.hwid.computer_name) << "\n";
    s << "  Machine GUID  : " << utf8(r.hwid.machine_guid) << "\n";
    s << "  Machine ID    : " << utf8(r.hwid.machine_id) << "\n\n";

    s << "[Accounts]\n";
    if (r.accounts.empty()) {
        s << "  (none found)\n";
    }
    for (const auto& a : r.accounts) {
        s << "  " << utf8(a.steamid64) << "  " << utf8(a.persona_name);
        if (!a.account_name.empty()) {
            s << " (" << utf8(a.account_name) << ")";
        }
        s << "\n    userdata bytes : " << a.userdata_bytes << "\n";
        s << "    localconfig    : " << (a.has_localconfig ? "present" : "missing") << "\n";
        s << "    loginusers     : " << (a.has_loginusers_entry ? "present" : "missing") << "\n";
    }
    s << "\n";

    s << "[Browser cookie databases]\n";
    if (r.browsers.empty()) {
        s << "  (none found)\n";
    }
    for (const auto& b : r.browsers) {
        s << "  " << utf8(b.browser) << " / " << utf8(b.profile) << "\n";
        s << "    " << b.cookies_db.string() << "\n";
    }
    s << "\n";

    s << "[Crash dumps]\n";
    if (r.crash_dumps.empty()) {
        s << "  (none found)\n";
    }
    for (const auto& c : r.crash_dumps) {
        s << "  " << c.dir.string() << "  count=" << c.count << "  bytes=" << c.total_bytes << "\n";
    }
    return s.str();
}

}  // namespace stc::core::audit
