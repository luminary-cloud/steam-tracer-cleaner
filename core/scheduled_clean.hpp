#pragma once

#include <filesystem>
#include <string>

namespace stc::core::scheduled {

constexpr wchar_t kTaskName[] = L"SteamTracerCleaner-LogonClean";

struct TaskInfo {
    bool registered = false;
    std::wstring next_run;     // populated by `query` if available
};

// Queries Task Scheduler for the registered logon task.
TaskInfo query();

// Registers a task that runs `exe_path --scheduled` whenever the current user logs on. Existing
// task with the same name is replaced. Returns true on success.
bool register_logon_task(const std::filesystem::path& exe_path);

// Removes the registered logon task. Returns true if it was deleted or did not exist.
bool unregister();

}  // namespace stc::core::scheduled
