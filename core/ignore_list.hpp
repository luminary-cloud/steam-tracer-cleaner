#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace stc::core {

struct IgnoreList {
    std::vector<std::wstring> preserved_account_ids;       // SteamID64 strings
    bool preserve_all_ssfn = false;                         // skip every ssfn file
    std::vector<std::wstring> preserved_paths;              // case-insensitive prefix match
    std::vector<std::wstring> preserved_registry_values;    // case-insensitive prefix match

    bool preserves_account(std::wstring_view steamid64) const;
    bool preserves_path(std::wstring_view path) const;
    bool preserves_registry(std::wstring_view full_key, std::wstring_view value_name) const;
    bool preserves_ssfn() const noexcept { return preserve_all_ssfn; }
};

std::optional<IgnoreList> load_ignore_list(const std::filesystem::path& json_path);

bool save_ignore_list(const IgnoreList& list, const std::filesystem::path& json_path);

// Returns a non-empty default ignore list with sane comments. Used the first time the user opens
// the Settings screen.
IgnoreList default_ignore_list();

}  // namespace stc::core
