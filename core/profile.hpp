#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace stc::core {

struct Profile {
    std::wstring name;
    std::wstring description;
    std::vector<std::string> target_ids;        // ids from targets.hpp
    std::vector<std::uint32_t> only_appids;     // empty = all; non-empty filters userdata to these AppIDs
    bool requires_confirmation = false;         // shown for "Full Wipe"
};

std::span<const Profile> built_in_profiles();

std::optional<Profile> load_profile(const std::filesystem::path& json_path);
bool save_profile(const Profile& p, const std::filesystem::path& json_path);

}  // namespace stc::core
