#pragma once

#include <cstdint>
#include <filesystem>
#include <string_view>
#include <vector>

namespace stc::core::crashdumps {

struct DumpFile {
    std::filesystem::path path;
    std::uintmax_t size_bytes = 0;
};

// Returns minidumps under %LOCALAPPDATA%\CrashDumps that look Steam-related, judging by filename.
// Steam writes filenames as "<exe>.<pid>.dmp" so prefix matching is reliable here.
std::vector<DumpFile> find_local_appdata_dumps();

// Returns true if `filename` has a Steam-related prefix (steam, cs2, csgo, gmod, hl2, l4d, dota,
// tf, srcds).
bool is_steam_related_filename(std::wstring_view filename);

}  // namespace stc::core::crashdumps
