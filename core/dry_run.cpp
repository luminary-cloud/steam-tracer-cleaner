#include "core/dry_run.hpp"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <cwctype>
#include <vector>

#include "core/steam_paths.hpp"
#include "core/vdf.hpp"
#include "platform/fs.hpp"
#include "platform/registry.hpp"

namespace stc::core {
namespace {

namespace fs_std = std::filesystem;

bool starts_with_ci(std::wstring_view a, std::wstring_view b) {
    if (b.size() > a.size()) {
        return false;
    }
    for (std::size_t i = 0; i < b.size(); ++i) {
        if (towlower(a[i]) != towlower(b[i])) {
            return false;
        }
    }
    return true;
}

bool target_is_loginusers_special(std::string_view id) { return id == "steam.loginusers"; }
bool target_is_ssfn(std::string_view id) { return id == "steam.ssfn"; }
bool target_is_userdata(std::string_view id) { return id == "steam.userdata"; }
bool target_is_autologin(std::string_view id) { return id == "steam.reg.autologin"; }
bool target_is_remote_clients(std::string_view id) { return id == "steam.remoteclients"; }
bool target_is_config_vdf(std::string_view id) { return id == "steam.config_vdf"; }

void compute_size(Operation& op) {
    if (op.kind == OpKind::RemoveFile || op.kind == OpKind::RemoveTree) {
        op.size_bytes = stc::platform::fsx::size_recursive(fs_std::path{op.target});
    }
}

bool ignore_filters_op(const Operation& op, std::string_view target_id, const IgnoreList* ignore) {
    if (!ignore) {
        return false;
    }
    if (!op.account_steamid64.empty() && ignore->preserves_account(op.account_steamid64)) {
        return true;
    }
    if (target_is_ssfn(target_id) && ignore->preserves_ssfn()) {
        return true;
    }
    if (op.kind == OpKind::RemoveFile || op.kind == OpKind::RemoveTree) {
        if (ignore->preserves_path(op.target)) {
            return true;
        }
    }
    if (op.kind == OpKind::RemoveRegistryKey || op.kind == OpKind::RemoveRegistryValue ||
        op.kind == OpKind::ClearRegistryValue) {
        if (ignore->preserves_registry(op.target, op.value_name)) {
            return true;
        }
    }
    return false;
}

void rewrite_loginusers(std::vector<PlanStep>& steps, const std::string& target_id,
                        const ResolveContext& ctx, const IgnoreList* ignore) {
    // Replace the file-delete op with one VdfRemoveChild per non-preserved account. If no
    // accounts to preserve, leave the file delete in place.
    if (!ignore || ignore->preserved_account_ids.empty()) {
        return;
    }

    auto vdf_path = ctx.install.config_dir / "loginusers.vdf";

    std::erase_if(steps, [&](const PlanStep& s) {
        return s.target_id == target_id && s.op.kind == OpKind::RemoveFile;
    });

    for (const auto& acc : ctx.accounts) {
        if (ignore->preserves_account(acc.steamid64)) {
            continue;
        }
        Operation op;
        op.kind = OpKind::VdfRemoveChild;
        op.target = vdf_path.wstring();
        op.value_name = acc.steamid64;
        op.account_steamid64 = acc.steamid64;
        steps.push_back(PlanStep{target_id, std::move(op)});
    }
}

void rewrite_autologin(std::vector<PlanStep>& steps, const std::string& target_id,
                       const ResolveContext& ctx, const IgnoreList* ignore) {
    // If the current AutoLoginUser belongs to a preserved account, leave it (and the surrounding
    // auto-login state) alone. Otherwise the default delete ops stand.
    if (!ignore || ignore->preserved_account_ids.empty()) {
        return;
    }
    auto current = stc::platform::reg::read_string(HKEY_CURRENT_USER, L"Software\\Valve\\Steam",
                                                   L"AutoLoginUser");
    if (!current || current->empty()) {
        return;
    }
    auto sid = stc::core::steam::resolve_auto_login(ctx.install, *current);
    if (sid.empty()) {
        return;
    }
    if (!ignore->preserves_account(sid)) {
        return;
    }
    // Strip the AutoLoginUser delete plus the values that are coupled to the auto-login flow.
    // RememberPassword and LastGameNameUsed describe the now-preserved account's state, so they
    // ride along. PseudoUUID stays cleaned: it's a machine GUID, not account data.
    std::erase_if(steps, [&](const PlanStep& s) {
        if (s.target_id != target_id || s.op.kind != OpKind::RemoveRegistryValue) {
            return false;
        }
        return s.op.value_name == L"AutoLoginUser" || s.op.value_name == L"RememberPassword" ||
               s.op.value_name == L"LastGameNameUsed";
    });
}

void rewrite_remote_clients(std::vector<PlanStep>& steps, const std::string& target_id,
                            const IgnoreList* ignore) {
    // remoteclients.vdf is keyed by an opaque client id and coplay_* filenames don't carry the
    // local user's SteamID64, so there's no clean way to split per-account. When any account is
    // preserved we skip the whole target rather than risk dropping data that belongs to it.
    if (!ignore || ignore->preserved_account_ids.empty()) {
        return;
    }
    std::erase_if(steps, [&](const PlanStep& s) { return s.target_id == target_id; });
}

void rewrite_config_vdf(std::vector<PlanStep>& steps, const std::string& target_id,
                        const ResolveContext& ctx, const IgnoreList* ignore) {
    if (!ignore || ignore->preserved_account_ids.empty()) {
        return;
    }
    auto vdf_path = ctx.install.config_dir / "config.vdf";
    auto doc = vdf::load(vdf_path);
    if (!doc || !doc->root || !doc->root->is_object()) {
        return;
    }

    auto* software = doc->root->find(L"Software");
    auto* valve = software ? software->find(L"Valve") : nullptr;
    auto* steam_node = valve ? valve->find(L"Steam") : nullptr;
    if (!steam_node || !steam_node->is_object()) {
        return;
    }

    std::erase_if(steps, [&](const PlanStep& s) {
        return s.target_id == target_id && s.op.kind == OpKind::RemoveFile;
    });

    auto* accounts_node = steam_node->find(L"Accounts");
    if (accounts_node != nullptr && accounts_node->is_object()) {
        for (const auto& entry : accounts_node->children()) {
            if (!entry.second || !entry.second->is_object()) {
                continue;
            }
            auto* sid_node = entry.second->find(L"SteamID");
            if (sid_node == nullptr || !sid_node->is_value()) {
                continue;
            }
            const std::wstring& sid64 = sid_node->value();
            if (ignore->preserves_account(sid64)) {
                continue;
            }
            Operation op;
            op.kind = OpKind::VdfRemoveChild;
            op.target = vdf_path.wstring();
            op.value_name = L"Software\\Valve\\Steam\\Accounts\\";
            op.value_name += entry.first;
            op.account_steamid64 = sid64;
            steps.push_back(PlanStep{target_id, std::move(op)});
        }
    }

    // The ConnectCache walk used to be a separate helper that returned a SteamID64 from a hex key.
    // Merged in here after the surgical-edit refactor because the two loops shared too much state
    // and threading the IgnoreList plus the steps vector through a free function felt heavier than
    // it needed to be.
    auto* cache_node = steam_node->find(L"ConnectCache");
    if (cache_node != nullptr && cache_node->is_object()) {
        for (const auto& entry : cache_node->children()) {
            const std::wstring& hex_key = entry.first;
            if (hex_key.empty()) {
                continue;
            }
            std::uint64_t parsed = 0;
            try {
                std::wstring tmp{hex_key};
                parsed = std::stoull(tmp, nullptr, 16);
            } catch (...) {
                continue;
            }
            std::wstring sid64;
            if (parsed <= 0xFFFFFFFFULL) {
                // 8-char hex = 32-bit account id; the rest of Steam works in SteamID64.
                sid64 = stc::core::steam::account_id_to_steamid64(static_cast<std::uint32_t>(parsed));
            } else {
                sid64 = std::to_wstring(parsed);
            }
            if (sid64.empty() || ignore->preserves_account(sid64)) {
                continue;
            }
            Operation op;
            op.kind = OpKind::VdfRemoveChild;
            op.target = vdf_path.wstring();
            op.value_name = L"Software\\Valve\\Steam\\ConnectCache\\";
            op.value_name += hex_key;
            op.account_steamid64 = std::move(sid64);
            steps.push_back(PlanStep{target_id, std::move(op)});
        }
    }
}

void rewrite_userdata_for_appids(std::vector<PlanStep>& steps, const std::string& target_id,
                                 const ResolveContext& ctx, std::span<const std::uint32_t> appids) {
    if (appids.empty()) {
        return;
    }
    // Replace each whole-userdata-folder op with per-AppID subdir ops.
    std::erase_if(steps, [&](const PlanStep& s) {
        return s.target_id == target_id && s.op.kind == OpKind::RemoveTree;
    });
    for (const auto& acc : ctx.accounts) {
        for (std::uint32_t appid : appids) {
            auto sub = acc.userdata_path / std::to_wstring(appid);
            std::error_code ec;
            if (!fs_std::is_directory(sub, ec)) {
                continue;
            }
            Operation op;
            op.kind = OpKind::RemoveTree;
            op.target = sub.wstring();
            op.account_steamid64 = acc.steamid64;
            steps.push_back(PlanStep{target_id, std::move(op)});
        }
    }
}

}  // namespace

Plan build_plan(std::span<const Target* const> targets, const ResolveContext& ctx,
                const PlanOptions& opts) {
    Plan plan;

    for (const Target* t : targets) {
        if (!t || !t->resolve) {
            continue;
        }
        std::vector<PlanStep> local;
        for (auto& op : t->resolve(ctx)) {
            if (ignore_filters_op(op, t->id, opts.ignore)) {
                continue;
            }
            local.push_back(PlanStep{t->id, std::move(op)});
        }

        if (target_is_loginusers_special(t->id)) {
            rewrite_loginusers(local, t->id, ctx, opts.ignore);
        }
        if (target_is_autologin(t->id)) {
            rewrite_autologin(local, t->id, ctx, opts.ignore);
        }
        if (target_is_remote_clients(t->id)) {
            rewrite_remote_clients(local, t->id, opts.ignore);
        }
        if (target_is_config_vdf(t->id)) {
            rewrite_config_vdf(local, t->id, ctx, opts.ignore);
        }
        if (target_is_userdata(t->id) && !opts.only_appids.empty()) {
            rewrite_userdata_for_appids(local, t->id, ctx, opts.only_appids);
        }

        for (auto& step : local) {
            compute_size(step.op);
            plan.total_bytes += step.op.size_bytes;
            if (step.op.kind == OpKind::RemoveFile) {
                plan.total_file_count += 1;
            } else if (step.op.kind == OpKind::RemoveTree) {
                plan.total_file_count += stc::platform::fsx::file_count_recursive(
                    fs_std::path{step.op.target});
            }
            plan.steps.push_back(std::move(step));
        }
    }

    return plan;
}

Plan build_plan_by_ids(std::span<const std::string> target_ids, const ResolveContext& ctx,
                       const PlanOptions& opts) {
    std::vector<const Target*> resolved;
    resolved.reserve(target_ids.size());
    for (const auto& id : target_ids) {
        if (auto* t = find_target(id)) {
            resolved.push_back(t);
        }
    }
    return build_plan(std::span<const Target* const>{resolved.data(), resolved.size()}, ctx, opts);
}

}  // namespace stc::core
