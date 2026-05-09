#pragma once

#include <windows.h>

namespace stc::core::steam_lifecycle {

struct CloseResult {
    int steam_closed = 0;
    int games_closed = 0;
};

CloseResult close_steam_and_games(DWORD wait_ms = 5000);

}  // namespace stc::core::steam_lifecycle
