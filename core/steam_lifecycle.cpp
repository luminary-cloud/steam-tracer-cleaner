#include "core/steam_lifecycle.hpp"

#include <spdlog/spdlog.h>

#include <array>
#include <string_view>

#include "core/appid_map.hpp"
#include "platform/process.hpp"

namespace stc::core::steam_lifecycle {
namespace {

constexpr std::array<std::wstring_view, 9> kGameExes = {{
    L"cs2.exe",
    L"csgo.exe",
    L"dota2.exe",
    L"hl2.exe",
    L"gmod.exe",
    L"left4dead2.exe",
    L"tf_win64.exe",
    L"srcds.exe",
    L"vrserver.exe",
}};

int close_all_named(std::wstring_view exe, DWORD wait_ms) {
    auto pids = stc::platform::proc::pids_by_name(exe);
    for (DWORD pid : pids) {
        stc::platform::proc::gracefully_close(pid, wait_ms);
    }
    return static_cast<int>(pids.size());
}

}  // namespace

CloseResult close_steam_and_games(DWORD wait_ms) {
    CloseResult r;
    for (auto exe : kGameExes) {
        r.games_closed += close_all_named(exe, wait_ms);
    }
    r.steam_closed = close_all_named(L"steam.exe", wait_ms);
    if (r.games_closed || r.steam_closed) {
        spdlog::info("Closed {} game(s) and {} Steam process(es)", r.games_closed, r.steam_closed);
    }
    return r;
}

int close_games_for_appid(std::uint32_t appid, DWORD wait_ms) {
    const auto* g = stc::core::appid::find_by_appid(appid);
    if (g == nullptr || g->exe_name.empty()) {
        return 0;
    }
    int closed = close_all_named(g->exe_name, wait_ms);
    if (closed > 0) {
        // exe_name is ASCII; narrow cast avoids dragging in WideCharToMultiByte.
        std::string narrow;
        narrow.reserve(g->exe_name.size());
        for (wchar_t c : g->exe_name) {
            narrow.push_back(static_cast<char>(c));
        }
        spdlog::info("Closed {} instance(s) of {} for appid {}", closed, narrow, appid);
    }
    return closed;
}

}  // namespace stc::core::steam_lifecycle
