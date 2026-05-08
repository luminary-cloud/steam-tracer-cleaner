#include "core/targets.hpp"

#include <algorithm>
#include <vector>

#include "core/browser_cookies.hpp"
#include "core/crash_dumps.hpp"

namespace stc::core {
namespace {

namespace fs_std = std::filesystem;

std::wstring path_to_w(const fs_std::path& p) { return p.wstring(); }

void push_dir(std::vector<Operation>& out, const fs_std::path& p) {
    Operation op;
    op.kind = OpKind::RemoveTree;
    op.target = path_to_w(p);
    out.push_back(std::move(op));
}

void push_file(std::vector<Operation>& out, const fs_std::path& p) {
    Operation op;
    op.kind = OpKind::RemoveFile;
    op.target = path_to_w(p);
    out.push_back(std::move(op));
}

void push_glob_files(std::vector<Operation>& out, const fs_std::path& dir, std::wstring_view prefix,
                     std::wstring_view suffix = L"") {
    std::error_code ec;
    if (!fs_std::is_directory(dir, ec)) {
        return;
    }
    for (auto it = fs_std::directory_iterator(dir, ec); !ec && it != fs_std::end(it);
         it.increment(ec)) {
        if (!it->is_regular_file(ec)) {
            continue;
        }
        auto name = it->path().filename().wstring();
        bool prefix_ok = prefix.empty() || name.starts_with(prefix);
        bool suffix_ok = suffix.empty() || name.ends_with(suffix);
        if (prefix_ok && suffix_ok) {
            push_file(out, it->path());
        }
    }
}

void push_reg_value(std::vector<Operation>& out, std::wstring_view full_key, std::wstring_view value) {
    Operation op;
    op.kind = OpKind::RemoveRegistryValue;
    op.target = std::wstring{full_key};
    op.value_name = std::wstring{value};
    out.push_back(std::move(op));
}

void push_reg_key(std::vector<Operation>& out, std::wstring_view full_key,
                  std::wstring_view account = L"") {
    Operation op;
    op.kind = OpKind::RemoveRegistryKey;
    op.target = std::wstring{full_key};
    op.account_steamid64 = std::wstring{account};
    out.push_back(std::move(op));
}

std::vector<Operation> resolve_dumps(const ResolveContext& ctx) {
    // Originally went through push_glob_files for the *.dmp sweep but the suffix-only matching
    // also caught e.g. ".dmpbackup" on one machine, hand-rolling it here.
    std::vector<Operation> ops;

    Operation dump_dir;
    dump_dir.kind = OpKind::RemoveTree;
    dump_dir.target = (ctx.install.install_path / "dumps").wstring();
    ops.push_back(std::move(dump_dir));

    std::error_code ec;
    if (fs_std::is_directory(ctx.install.install_path, ec)) {
        for (auto it = fs_std::directory_iterator(ctx.install.install_path, ec);
             !ec && it != fs_std::end(it); it.increment(ec)) {
            if (!it->is_regular_file(ec)) {
                continue;
            }
            const auto name = it->path().filename().wstring();
            if (name.size() < 4) {
                continue;
            }
            const auto suffix = name.substr(name.size() - 4);
            if (suffix == L".dmp") {
                Operation op;
                op.kind = OpKind::RemoveFile;
                op.target = it->path().wstring();
                ops.push_back(std::move(op));
            }
        }
    }
    return ops;
}

std::vector<Operation> resolve_logs(const ResolveContext& ctx) {
    std::vector<Operation> ops;
    push_dir(ops, ctx.install.install_path / "logs");
    push_glob_files(ops, ctx.install.install_path, L"GameOverlayUI.exe.log");
    push_glob_files(ops, ctx.install.install_path, L"GameOverlayRenderer", L".log");
    push_glob_files(ops, ctx.install.install_path, L"", L".last");
    return ops;
}

std::vector<Operation> resolve_appcache(const ResolveContext& ctx) {
    std::vector<Operation> ops;
    push_dir(ops, ctx.install.appcache_dir / "httpcache");
    push_dir(ops, ctx.install.appcache_dir / "librarycache");
    push_file(ops, ctx.install.appcache_dir / "appinfo.vdf");
    push_file(ops, ctx.install.appcache_dir / "packageinfo.vdf");
    return ops;
}

std::vector<Operation> resolve_depotcache(const ResolveContext& ctx) {
    std::vector<Operation> ops;
    for (const auto& lib : ctx.libraries) {
        push_dir(ops, lib.parent_path() / "depotcache");
    }
    return ops;
}

std::vector<Operation> resolve_shadercache(const ResolveContext& ctx) {
    std::vector<Operation> ops;
    for (const auto& lib : ctx.libraries) {
        std::error_code ec;
        auto sc = lib / "shadercache";
        if (fs_std::is_directory(sc, ec)) {
            push_dir(ops, sc);
        }
    }
    return ops;
}

std::vector<Operation> resolve_workshop_temp(const ResolveContext& ctx) {
    std::vector<Operation> ops;
    for (const auto& lib : ctx.libraries) {
        push_dir(ops, lib / "workshop" / "temp");
        push_dir(ops, lib / "workshop" / "downloads");
    }
    return ops;
}

std::vector<Operation> resolve_htmlcache(const ResolveContext& ctx) {
    std::vector<Operation> ops;
    push_dir(ops, ctx.install.htmlcache_dir);
    push_dir(ops, ctx.install.config_dir / "htmlcache");
    return ops;
}

std::vector<Operation> resolve_loginusers_vdf(const ResolveContext& ctx) {
    // The planner replaces this file-delete with VdfRemoveChild ops for non-preserved accounts
    // when the ignore list has any preserved ids.
    std::vector<Operation> ops;
    push_file(ops, ctx.install.config_dir / "loginusers.vdf");
    return ops;
}

std::vector<Operation> resolve_ssfn_files(const ResolveContext& ctx) {
    std::vector<Operation> ops;
    push_glob_files(ops, ctx.install.install_path, L"ssfn");
    return ops;
}

std::vector<Operation> resolve_auto_login_value(const ResolveContext&) {
    std::vector<Operation> ops;
    push_reg_value(ops, L"HKCU\\Software\\Valve\\Steam", L"AutoLoginUser");
    push_reg_value(ops, L"HKCU\\Software\\Valve\\Steam", L"RememberPassword");
    push_reg_value(ops, L"HKCU\\Software\\Valve\\Steam", L"LastGameNameUsed");
    push_reg_value(ops, L"HKCU\\Software\\Valve\\Steam", L"PseudoUUID");
    return ops;
}

std::vector<Operation> resolve_users_subtree(const ResolveContext& ctx) {
    // Per-account: the cleaner filters this via the ignore list before deleting.
    std::vector<Operation> ops;
    for (const auto& acc : ctx.accounts) {
        std::wstring full = L"HKCU\\Software\\Valve\\Steam\\Users\\" + acc.steamid64;
        push_reg_key(ops, full, acc.steamid64);
    }
    return ops;
}

std::vector<Operation> resolve_active_process(const ResolveContext&) {
    std::vector<Operation> ops;
    push_reg_key(ops, L"HKCU\\Software\\Valve\\Steam\\ActiveProcess");
    return ops;
}

std::vector<Operation> resolve_remote_clients(const ResolveContext& ctx) {
    std::vector<Operation> ops;
    push_file(ops, ctx.install.config_dir / "remoteclients.vdf");
    push_glob_files(ops, ctx.install.config_dir, L"coplay_");
    return ops;
}

std::vector<Operation> resolve_avatarcache(const ResolveContext& ctx) {
    // Per-file rather than RemoveTree because we used to skip a single avatar that contained the
    // user's custom-uploaded persona pic. The whitelist got removed but the per-file walk stuck.
    std::vector<Operation> ops;
    auto root = ctx.install.config_dir / "avatarcache";
    std::error_code ec;
    if (!fs_std::is_directory(root, ec)) {
        return ops;
    }
    for (auto it = fs_std::directory_iterator(root, ec); !ec && it != fs_std::end(it);
         it.increment(ec)) {
        if (it->is_regular_file(ec)) {
            Operation op;
            op.kind = OpKind::RemoveFile;
            op.target = it->path().wstring();
            ops.push_back(std::move(op));
        }
    }
    return ops;
}

std::vector<Operation> resolve_userdata_full(const ResolveContext& ctx) {
    // Per-account, ignore list filters by account_steamid64 before delete.
    std::vector<Operation> ops;
    for (const auto& acc : ctx.accounts) {
        Operation op;
        op.kind = OpKind::RemoveTree;
        op.target = path_to_w(acc.userdata_path);
        op.account_steamid64 = acc.steamid64;
        ops.push_back(std::move(op));
    }
    return ops;
}

std::vector<Operation> resolve_browser_steam_cookies(const ResolveContext&) {
    std::vector<Operation> ops;
    for (const auto& db : stc::core::browser::discover_cookie_dbs()) {
        Operation op;
        op.kind = OpKind::ClearBrowserSteamCookies;
        op.target = db.db_path.wstring();
        op.value_name = db.schema == stc::core::browser::Schema::Firefox ? L"firefox" : L"chromium";
        ops.push_back(std::move(op));
    }
    return ops;
}

std::vector<Operation> resolve_crash_local_appdata(const ResolveContext&) {
    std::vector<Operation> ops;
    for (const auto& d : stc::core::crashdumps::find_local_appdata_dumps()) {
        Operation op;
        op.kind = OpKind::RemoveFile;
        op.target = d.path.wstring();
        ops.push_back(std::move(op));
    }
    return ops;
}

std::vector<Operation> resolve_controller_configs(const ResolveContext& ctx) {
    std::vector<Operation> ops;
    for (const auto& acc : ctx.accounts) {
        auto base = acc.userdata_path / "config" / "controller_configs";
        Operation op;
        op.kind = OpKind::RemoveTree;
        op.target = path_to_w(base);
        op.account_steamid64 = acc.steamid64;
        ops.push_back(std::move(op));
    }
    push_dir(ops, ctx.install.install_path / "steamapps" / "common" / "Steam Controller Configs");
    return ops;
}

std::vector<Operation> resolve_config_vdf(const ResolveContext& ctx) {
    std::vector<Operation> ops;
    push_file(ops, ctx.install.config_dir / "config.vdf");
    return ops;
}

std::vector<Operation> resolve_appcache_stats(const ResolveContext& ctx) {
    std::vector<Operation> ops;
    push_dir(ops, ctx.install.appcache_dir / "stats");
    return ops;
}

std::vector<Operation> resolve_tenfoot_httpcache(const ResolveContext& ctx) {
    std::vector<Operation> ops;
    push_dir(ops, ctx.install.install_path / "tenfoot" / "config" / "httpcache");
    return ops;
}

std::vector<Operation> resolve_overlayhtmlcache(const ResolveContext& ctx) {
    std::vector<Operation> ops;
    push_dir(ops, ctx.install.config_dir / "overlayhtmlcache");
    return ops;
}

std::vector<Operation> resolve_steamapps_downloading(const ResolveContext& ctx) {
    std::vector<Operation> ops;
    for (const auto& lib : ctx.libraries) {
        push_dir(ops, lib / "downloading");
    }
    return ops;
}

std::vector<Operation> resolve_reg_apps(const ResolveContext&) {
    std::vector<Operation> ops;
    push_reg_key(ops, L"HKCU\\Software\\Valve\\Steam\\Apps");
    return ops;
}

std::vector<Operation> resolve_reg_url_handler(const ResolveContext&) {
    std::vector<Operation> ops;
    push_reg_key(ops, L"HKCR\\steam");
    push_reg_key(ops, L"HKCU\\Software\\Classes\\steam");
    return ops;
}

std::vector<Operation> resolve_userdata_shortcuts(const ResolveContext& ctx) {
    std::vector<Operation> ops;
    for (const auto& acc : ctx.accounts) {
        Operation op;
        op.kind = OpKind::RemoveFile;
        op.target = path_to_w(acc.userdata_path / "config" / "shortcuts.vdf");
        op.account_steamid64 = acc.steamid64;
        ops.push_back(std::move(op));
    }
    return ops;
}

std::vector<Operation> resolve_userdata_screenshots(const ResolveContext& ctx) {
    std::vector<Operation> ops;
    const std::size_t n = ctx.accounts.size();
    for (std::size_t i = 0; i < n; ++i) {
        const auto& acc = ctx.accounts[i];
        if (acc.userdata_path.empty()) {
            // Should never happen, enumerate_accounts skips empty paths, but I ran into a
            // crash here once when userdata enumeration ran before the install was fully
            // detected.
            continue;
        }
        Operation op;
        op.kind = OpKind::RemoveTree;
        op.target = path_to_w(acc.userdata_path / "760" / "remote");
        op.account_steamid64 = acc.steamid64;
        ops.push_back(std::move(op));
    }
    return ops;
}

std::vector<Operation> resolve_userdata_inventory_cache(const ResolveContext& ctx) {
    std::vector<Operation> ops;
    for (auto a : ctx.accounts) {
        auto target = path_to_w(a.userdata_path / "inventorymsgcache");
        ops.push_back(Operation{OpKind::RemoveTree, target, L"", a.steamid64, 0});
    }
    return ops;
}

std::vector<Operation> resolve_userdata_librarycache(const ResolveContext& ctx) {
    std::vector<Operation> ops;
    for (const auto& acc : ctx.accounts) {
        auto target = acc.userdata_path / "config" / "librarycache";
        std::error_code ec;
        if (!fs_std::is_directory(target, ec)) {
            continue;
        }
        Operation op;
        op.kind = OpKind::RemoveTree;
        op.target = path_to_w(target);
        op.account_steamid64 = acc.steamid64;
        ops.push_back(std::move(op));
    }
    return ops;
}

std::vector<Operation> resolve_userdata_sharedconfig(const ResolveContext& ctx) {
    std::vector<Operation> ops;
    for (const auto& acc : ctx.accounts) {
        Operation op;
        op.kind = OpKind::RemoveFile;
        op.target = path_to_w(acc.userdata_path / "7" / "remote" / "sharedconfig.vdf");
        op.account_steamid64 = acc.steamid64;
        ops.push_back(std::move(op));
    }
    return ops;
}

std::vector<Operation> resolve_appmanifests(const ResolveContext& ctx) {
    std::vector<Operation> ops;
    for (const auto& lib : ctx.libraries) {
        push_glob_files(ops, lib, L"appmanifest_", L".acf");
    }
    return ops;
}

const std::vector<Target>& catalog() {
    static const std::vector<Target> targets = [] {
        std::vector<Target> t;
        t.push_back(Target{"steam.dumps", L"Crash dumps",
                           L"Steam-side minidumps and ETL traces under <install>/dumps and *.dmp at the root.",
                           TargetCategory::CrashDump, &resolve_dumps});
        t.push_back(Target{"steam.logs", L"Logs",
                           L"<install>/logs plus GameOverlay logs and *.last status files.",
                           TargetCategory::Log, &resolve_logs});
        t.push_back(Target{"steam.appcache", L"Appcache (httpcache + librarycache)",
                           L"appinfo.vdf, packageinfo.vdf, library thumbnails, web image cache.",
                           TargetCategory::Cache, &resolve_appcache});
        t.push_back(Target{"steam.depotcache", L"Depot manifests",
                           L".manifest files leak owned-game history. Deleted across every Steam library drive.",
                           TargetCategory::Cache, &resolve_depotcache});
        t.push_back(Target{"steam.shadercache", L"Shader cache",
                           L"Per-AppID compiled shader pipelines. Rebuilds on next launch.",
                           TargetCategory::Cache, &resolve_shadercache});
        t.push_back(Target{"steam.workshop_temp", L"Workshop temp + downloads",
                           L"Partial workshop downloads.", TargetCategory::Cache,
                           &resolve_workshop_temp});
        t.push_back(Target{"steam.htmlcache", L"HTML / CEF cache",
                           L"%LOCALAPPDATA%/Steam/htmlcache plus old <install>/config/htmlcache. "
                           L"Holds Steam web cookies and store sessions.",
                           TargetCategory::BrowserResidue, &resolve_htmlcache});
        t.push_back(Target{"steam.loginusers", L"loginusers.vdf",
                           L"Account list with AutoLogin tokens. Surgically edited so preserved "
                           L"accounts survive.",
                           TargetCategory::AccountResidue, &resolve_loginusers_vdf});
        t.push_back(Target{"steam.ssfn", L"Steam Guard sentry files (ssfn*)",
                           L"Per-account 2FA bypass files. Set 'preserved_ssfn_files' in ignore.json "
                           L"to skip.",
                           TargetCategory::AccountResidue, &resolve_ssfn_files});
        t.push_back(Target{"steam.reg.autologin", L"HKCU AutoLoginUser + friends",
                           L"AutoLoginUser, RememberPassword, LastGameNameUsed, PseudoUUID under "
                           L"HKCU\\Software\\Valve\\Steam.",
                           TargetCategory::AccountResidue, &resolve_auto_login_value});
        t.push_back(Target{"steam.reg.users", L"HKCU\\...\\Steam\\Users\\<SteamID64>",
                           L"Per-account subtree under HKCU. Preserved accounts are skipped.",
                           TargetCategory::AccountResidue, &resolve_users_subtree});
        t.push_back(Target{"steam.reg.activeprocess", L"HKCU\\...\\Steam\\ActiveProcess",
                           L"Active Steam process state. Recreated at next launch.",
                           TargetCategory::AccountResidue, &resolve_active_process});
        t.push_back(Target{"steam.remoteclients", L"Remote clients + coplay",
                           L"remoteclients.vdf and coplay_* friend session caches.",
                           TargetCategory::AccountResidue, &resolve_remote_clients});
        t.push_back(Target{"steam.avatarcache", L"Avatar cache",
                           L"Cached friend / persona avatars.", TargetCategory::Cache,
                           &resolve_avatarcache});
        t.push_back(Target{"steam.userdata", L"Userdata folders",
                           L"Per-account game saves, cloud sync state, settings. Filtered by ignore list.",
                           TargetCategory::GameData, &resolve_userdata_full});
        t.push_back(Target{"steam.controller_configs", L"Steam Input bindings",
                           L"Per-account controller layouts under userdata\\<id>\\config\\controller_configs.",
                           TargetCategory::ControllerResidue, &resolve_controller_configs});
        t.push_back(Target{"browser.steam_cookies", L"Browser Steam cookies",
                           L"Removes steamcommunity.com and steampowered.com cookies from Chrome, "
                           L"Edge, Brave, Vivaldi, and Firefox profiles. Browser must be closed.",
                           TargetCategory::BrowserResidue, &resolve_browser_steam_cookies});
        t.push_back(Target{"crash.local_appdata", L"Steam-related crash dumps",
                           L"Removes steam.exe / cs2.exe / csgo.exe / gmod.exe etc. dumps from "
                           L"%LOCALAPPDATA%\\CrashDumps.",
                           TargetCategory::CrashDump, &resolve_crash_local_appdata});
        t.push_back(Target{"steam.config_vdf", L"config.vdf",
                           L"Maps every account ever logged in to its sentry-file filename. Steam "
                           L"regenerates a fresh empty config on next launch.",
                           TargetCategory::AccountResidue, &resolve_config_vdf});
        t.push_back(Target{"steam.appcache_stats", L"Appcache stats",
                           L"Per-game stats binary cache under <install>/appcache/stats.",
                           TargetCategory::Cache, &resolve_appcache_stats});
        t.push_back(Target{"steam.tenfoot_httpcache", L"Big Picture (tenfoot) HTTP cache",
                           L"<install>/tenfoot/config/httpcache. Separate from the regular "
                           L"htmlcache; covers Big Picture mode's CEF cache.",
                           TargetCategory::BrowserResidue, &resolve_tenfoot_httpcache});
        t.push_back(Target{"steam.overlayhtmlcache", L"Steam Overlay HTML cache",
                           L"<install>/config/overlayhtmlcache. CEF cache used by the in-game "
                           L"overlay browser.",
                           TargetCategory::BrowserResidue, &resolve_overlayhtmlcache});
        t.push_back(Target{"steam.steamapps_downloading", L"Partial downloads",
                           L"<library>/steamapps/downloading on every Steam library drive. "
                           L"Resumeable game-download chunks.",
                           TargetCategory::Cache, &resolve_steamapps_downloading});
        t.push_back(Target{"steam.reg.apps", L"HKCU\\...\\Steam\\Apps",
                           L"Per-AppID registry subtree (last-played, language, install state). "
                           L"Wholesale delete; Steam recreates entries as needed.",
                           TargetCategory::AccountResidue, &resolve_reg_apps});
        t.push_back(Target{"steam.reg.url_handler", L"steam:// URL handler",
                           L"Removes HKCR\\steam and HKCU\\Software\\Classes\\steam. Steam "
                           L"re-registers the handler on next launch.",
                           TargetCategory::AccountResidue, &resolve_reg_url_handler});
        t.push_back(Target{"steam.userdata_shortcuts", L"Non-Steam game shortcuts",
                           L"userdata\\<id>\\config\\shortcuts.vdf. Lists external apps you added "
                           L"to your library. Per-account, ignore-list filtered.",
                           TargetCategory::AccountResidue, &resolve_userdata_shortcuts});
        t.push_back(Target{"steam.userdata_screenshots", L"Screenshots",
                           L"userdata\\<id>\\760\\remote. Steam screenshot uploads. Per-account.",
                           TargetCategory::GameData, &resolve_userdata_screenshots});
        t.push_back(Target{"steam.userdata_inventory_cache", L"Inventory message cache",
                           L"userdata\\<id>\\inventorymsgcache. Per-account, ignore-list filtered.",
                           TargetCategory::Cache, &resolve_userdata_inventory_cache});
        t.push_back(Target{"steam.userdata_librarycache", L"Per-account library cache",
                           L"userdata\\<id>\\config\\librarycache. Per-account thumbnails and "
                           L"library metadata.",
                           TargetCategory::Cache, &resolve_userdata_librarycache});
        t.push_back(Target{"steam.userdata_sharedconfig", L"sharedconfig.vdf",
                           L"userdata\\<id>\\7\\remote\\sharedconfig.vdf. Holds library categories, "
                           L"hidden-game flags, and tags. Per-account, ignore-list filtered.",
                           TargetCategory::AccountResidue, &resolve_userdata_sharedconfig});
        t.push_back(Target{"steam.appmanifests", L"App manifests (installed-game records)",
                           L"<library>/steamapps/appmanifest_*.acf across every library. Removes "
                           L"the record of which games are installed. Opt-in: Steam will see "
                           L"games as uninstalled until you re-add them.",
                           TargetCategory::GameData, &resolve_appmanifests});
        return t;
    }();
    return targets;
}

}  // namespace

std::span<const Target> all_targets() {
    const auto& c = catalog();
    return {c.data(), c.size()};
}

const Target* find_target(std::string_view id) {
    const auto& c = catalog();
    auto it = std::find_if(c.begin(), c.end(), [&](const Target& t) { return t.id == id; });
    return it == c.end() ? nullptr : &*it;
}

}  // namespace stc::core
