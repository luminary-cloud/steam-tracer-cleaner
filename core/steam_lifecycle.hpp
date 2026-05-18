#pragma once

#include <windows.h>

#include <cstdint>

namespace stc::core::steam_lifecycle {

struct CloseResult {
    int steam_closed = 0;
    int games_closed = 0;
};

CloseResult close_steam_and_games(DWORD wait_ms = 5000);

// Closes only the exe mapped to `appid` via appid_map (e.g. 730 → cs2.exe). Steam stays up;
// config installs touch per-game files, not Steam's own state. Unknown appid → 0.
int close_games_for_appid(std::uint32_t appid, DWORD wait_ms = 5000);

}  // namespace stc::core::steam_lifecycle
