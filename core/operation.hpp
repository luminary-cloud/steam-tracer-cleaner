#pragma once

#include <cstdint>
#include <string>

namespace stc::core {

// Values use `Remove` instead of `Delete` because Win32 defines DeleteFile and RemoveDirectory as
// macros that would rewrite the enum names at preprocessing time.
enum class OpKind {
    RemoveFile,
    RemoveTree,                // recursive directory delete
    RemoveRegistryValue,
    RemoveRegistryKey,
    VdfRemoveChild,            // target = vdf path, value_name = top-level child key to remove
    ClearRegistryValue,        // writes empty string instead of deleting; used for AutoLoginUser
                               // when partially preserving
    ClearBrowserSteamCookies,  // target = cookies db path, value_name = "chromium" | "firefox"
};

// A single concrete cleanup action. Targets resolve to one or more of these.
struct Operation {
    OpKind kind;

    // Filesystem path for RemoveFile / RemoveTree.
    // Registry ops: full path including hive prefix, e.g. "HKCU\\Software\\Valve\\Steam".
    std::wstring target;

    // Value name for registry value ops. Empty for other kinds.
    std::wstring value_name;

    // SteamID64 of the account this op belongs to, or empty if global. The ignore list uses this
    // to preserve account-scoped ops.
    std::wstring account_steamid64;

    // Populated by the planner for the dry-run summary.
    std::uint64_t size_bytes = 0;
};

}  // namespace stc::core
