#pragma once

#include <functional>
#include <span>
#include <string>
#include <vector>

#include "core/operation.hpp"
#include "core/steam_paths.hpp"

namespace stc::core {

enum class TargetCategory {
    Cache,
    Log,
    AccountResidue,
    BrowserResidue,
    ControllerResidue,
    GameData,
    CrashDump,
    Hwid,  // for the audit screen, never deleted
};

struct ResolveContext {
    const stc::core::steam::InstallInfo& install;
    const std::vector<stc::core::steam::AccountInfo>& accounts;
    const std::vector<std::filesystem::path>& libraries;
};

struct Target {
    std::string id;                // stable, e.g. "steam.htmlcache"
    std::wstring display_name;     // shown in UI
    std::wstring description;      // tooltip text
    TargetCategory category;

    // Resolves this target to concrete operations against the given context.
    std::function<std::vector<Operation>(const ResolveContext&)> resolve;
};

// Returns the built-in target catalog. Stable across calls in the same process.
std::span<const Target> all_targets();

// Lookup by id. Returns nullptr if not found.
const Target* find_target(std::string_view id);

}  // namespace stc::core
