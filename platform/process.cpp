#include "platform/process.hpp"

#include <windows.h>

#include <tlhelp32.h>

#include <spdlog/spdlog.h>

#include <algorithm>
#include <cwchar>

namespace stc::platform::proc {
namespace {

bool eq_ci(std::wstring_view a, std::wstring_view b) {
    if (a.size() != b.size()) {
        return false;
    }
    for (std::size_t i = 0; i < a.size(); ++i) {
        if (towlower(a[i]) != towlower(b[i])) {
            return false;
        }
    }
    return true;
}

struct EnumCtx {
    DWORD pid;
    int posted;
};

BOOL CALLBACK post_close_proc(HWND hwnd, LPARAM lp) {
    auto* ctx = reinterpret_cast<EnumCtx*>(lp);
    DWORD owner = 0;
    GetWindowThreadProcessId(hwnd, &owner);
    if (owner == ctx->pid && IsWindowVisible(hwnd)) {
        PostMessageW(hwnd, WM_CLOSE, 0, 0);
        ++ctx->posted;
    }
    return TRUE;
}

}  // namespace

std::vector<DWORD> pids_by_name(std::wstring_view exe_name) {
    std::vector<DWORD> out;
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) {
        return out;
    }
    PROCESSENTRY32W pe{};
    pe.dwSize = sizeof(pe);
    if (Process32FirstW(snap, &pe)) {
        do {
            if (eq_ci(exe_name, pe.szExeFile)) {
                out.push_back(pe.th32ProcessID);
            }
        } while (Process32NextW(snap, &pe));
    }
    CloseHandle(snap);
    return out;
}

bool is_running(std::wstring_view exe_name) {
    return !pids_by_name(exe_name).empty();
}

bool gracefully_close(DWORD pid, DWORD wait_ms) {
    HANDLE h = OpenProcess(SYNCHRONIZE | PROCESS_TERMINATE, FALSE, pid);
    if (!h) {
        return false;
    }
    EnumCtx ctx{pid, 0};
    EnumWindows(post_close_proc, reinterpret_cast<LPARAM>(&ctx));

    // Steam takes ~1.5s to actually close after WM_CLOSE in my testing. The 5s default the
    // header gives us is fine; raising it any further just makes the cleaner UI feel laggy.
    DWORD wait = WaitForSingleObject(h, wait_ms);
    if (wait == WAIT_OBJECT_0) {
        CloseHandle(h);
        return true;
    }
    spdlog::warn("Graceful close timed out for pid {}, terminating", pid);
    TerminateProcess(h, 1);
    WaitForSingleObject(h, 2000);
    CloseHandle(h);
    return true;
}

}  // namespace stc::platform::proc
