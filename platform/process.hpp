#pragma once

#include <windows.h>

#include <string_view>
#include <vector>

namespace stc::platform::proc {

std::vector<DWORD> pids_by_name(std::wstring_view exe_name);
bool is_running(std::wstring_view exe_name);

// Tries WM_CLOSE on visible top-level windows owned by `pid`, then waits up to `wait_ms`. If the
// process is still alive, falls back to TerminateProcess. Returns true if the process is gone by
// the time we return.
bool gracefully_close(DWORD pid, DWORD wait_ms = 5000);

}  // namespace stc::platform::proc
