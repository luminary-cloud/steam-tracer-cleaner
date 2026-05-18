#pragma once

#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <vector>

#include "core/ignore_list.hpp"
#include "core/operation.hpp"
#include "core/steam_paths.hpp"
#include "core/targets.hpp"

namespace stc::core {

struct PlanStep {
    std::string target_id;
    Operation op;
};

struct Plan {
    std::vector<PlanStep> steps;
    std::uint64_t total_bytes = 0;
    std::uint64_t total_file_count = 0;
};

struct PlanOptions {
    const IgnoreList* ignore = nullptr;            // optional, applied at plan build time
    std::vector<std::uint32_t> only_appids;        // for "Game Reset" profile, empty = unrestricted
};

// Resolves the given targets against the context, applies the ignore list and the AppID filter,
// computes byte sizes, and returns a self-contained Plan that the cleaner can execute without the
// ignore list.
Plan build_plan(std::span<const Target* const> targets, const ResolveContext& ctx,
                const PlanOptions& opts);

// Convenience: resolve a list of target ids first.
Plan build_plan_by_ids(std::span<const std::string> target_ids, const ResolveContext& ctx,
                       const PlanOptions& opts);

struct AutoLoginRedirect {
    std::wstring account_name;   // value to write into HKCU\...\Steam\AutoLoginUser
    std::wstring steamid64;      // matching entry in loginusers.vdf, used to flip mostrecent
};

// Picks the preserved account to replace AutoLoginUser when the current auto-login account
// is being wiped. Prefers most_recent, otherwise the first preserved account with a non-empty
// account_name. Returns nullopt if none match.
std::optional<AutoLoginRedirect> pick_autologin_redirect(
    std::span<const stc::core::steam::AccountInfo> accounts, const IgnoreList& ignore);

}  // namespace stc::core
