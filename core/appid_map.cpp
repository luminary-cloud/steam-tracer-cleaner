#include "core/appid_map.hpp"

#include <algorithm>
#include <array>

namespace stc::core::appid {
namespace {

constexpr std::array<GameInfo, 14> kGames{{
    {730,    L"Counter-Strike Global Offensive", L"game/csgo",   L"Counter-Strike 2", true,  L"cs2.exe"},
    {570,    L"dota 2 beta",                     L"game/dota",   L"Dota 2",           true,  L"dota2.exe"},
    {440,    L"Team Fortress 2",                 L"tf",          L"Team Fortress 2",  false, L"tf_win64.exe"},
    {550,    L"Left 4 Dead 2",                   L"left4dead2",  L"Left 4 Dead 2",    false, L"left4dead2.exe"},
    {500,    L"Left 4 Dead",                     L"left4dead",   L"Left 4 Dead",      false, L"left4dead.exe"},
    {4000,   L"GarrysMod",                       L"garrysmod",   L"Garry's Mod",      false, L"gmod.exe"},
    {220,    L"Half-Life 2",                     L"hl2",         L"Half-Life 2",      false, L"hl2.exe"},
    {320,    L"Half-Life 2 Deathmatch",          L"hl2mp",       L"HL2: Deathmatch",  false, L"hl2.exe"},
    {17520,  L"Synergy",                         L"synergy",     L"Synergy",          false, L"hl2.exe"},
    {243750, L"Source SDK Base 2013 Multiplayer", L"hl2mp",      L"Source SDK 2013",  false, L"hl2.exe"},
    {17500,  L"Zombie Panic Source",             L"zps",         L"Zombie Panic",     false, L"hl2.exe"},
    {243730, L"Source SDK Base 2013 Singleplayer", L"hl2",       L"Source SDK 2013 SP", false, L"hl2.exe"},
    {730830, L"Insurgency",                      L"insurgency",  L"Insurgency 2014",  false, L"insurgency.exe"},
    {17710,  L"Nuclear Dawn",                    L"nucleardawn", L"Nuclear Dawn",     false, L"nucleardawn.exe"},
}};

}  // namespace

std::span<const GameInfo> all_games() noexcept {
    return {kGames.data(), kGames.size()};
}

const GameInfo* find_by_appid(std::uint32_t appid) noexcept {
    auto it = std::find_if(kGames.begin(), kGames.end(),
                           [&](const GameInfo& g) { return g.appid == appid; });
    return it == kGames.end() ? nullptr : &*it;
}

const GameInfo* find_by_install_folder(std::wstring_view install_folder) noexcept {
    auto it = std::find_if(kGames.begin(), kGames.end(), [&](const GameInfo& g) {
        return g.install_folder == install_folder;
    });
    return it == kGames.end() ? nullptr : &*it;
}

}  // namespace stc::core::appid
