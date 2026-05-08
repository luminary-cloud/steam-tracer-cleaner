#pragma once

#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace stc::core::appid {

struct GameInfo {
    std::uint32_t appid;
    std::wstring_view install_folder;     // steamapps/common/<install_folder>
    std::wstring_view mod_folder;         // <install_folder>/<mod_folder>/cfg/autoexec.cfg
    std::wstring_view display_name;
    bool is_source2;                      // CS2 / Dota 2
};

std::span<const GameInfo> all_games() noexcept;

const GameInfo* find_by_appid(std::uint32_t appid) noexcept;
const GameInfo* find_by_install_folder(std::wstring_view install_folder) noexcept;

}  // namespace stc::core::appid
