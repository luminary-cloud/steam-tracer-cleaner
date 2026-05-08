#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

#include "core/backup.hpp"
#include "core/dry_run.hpp"

namespace stc::core {

struct CleanResult {
    int succeeded = 0;
    int failed = 0;
    std::uint64_t bytes_freed = 0;
    std::vector<std::wstring> failure_messages;
};

struct CleanOptions {
    backup::Session* backup = nullptr;                     // null = no backup
    std::function<void(int done, int total)> on_progress;  // null = no callback
};

// Executes every step of the plan in order. Calls `on_progress` after each step. If a backup
// session is supplied, the relevant artifact is mirrored / exported before deletion.
CleanResult execute(const Plan& plan, const CleanOptions& opts);

}  // namespace stc::core
