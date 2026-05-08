#pragma once

#include <cstdint>
#include <set>
#include <vector>

#include "core/steam_paths.hpp"

namespace stc::ui::widgets {

// Renders a checkbox list of accounts. The picker shows persona + login name + SteamID64. Mutates
// `selected_account_ids` (uint32 SteamID3 form, matching userdata folder names) in place. Returns
// true if any selection changed.
bool account_picker(const std::vector<stc::core::steam::AccountInfo>& accounts,
                    std::set<std::uint32_t>& selected_account_ids);

}  // namespace stc::ui::widgets
