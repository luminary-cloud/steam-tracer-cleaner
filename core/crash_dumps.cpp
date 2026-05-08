#include "core/crash_dumps.hpp"

#include <array>
#include <cwctype>
#include <system_error>

#include "platform/paths.hpp"

namespace stc::core::crashdumps {
namespace {

constexpr std::array<std::wstring_view, 11> kPrefixes = {{
    L"steam",
    L"cs2",
    L"csgo",
    L"gmod",
    L"hl2",
    L"l4d",
    L"dota",
    L"tf",
    L"srcds",
    L"vrserver",
    L"vrclient",
}};

std::wstring lower(std::wstring_view in) {
    std::wstring out{in};
    for (auto& ch : out) {
        ch = static_cast<wchar_t>(towlower(ch));
    }
    return out;
}

}  // namespace

bool is_steam_related_filename(std::wstring_view filename) {
    auto lc = lower(filename);
    for (auto p : kPrefixes) {
        if (lc.starts_with(p)) {
            return true;
        }
    }
    return false;
}

std::vector<DumpFile> find_local_appdata_dumps() {
    std::vector<DumpFile> out;
    namespace fs_std = std::filesystem;
    auto root = stc::platform::local_appdata_dir() / "CrashDumps";
    std::error_code ec;
    if (!fs_std::is_directory(root, ec)) {
        return out;
    }
    for (auto it = fs_std::directory_iterator(root, ec); !ec && it != fs_std::end(it);
         it.increment(ec)) {
        if (!it->is_regular_file(ec)) {
            continue;
        }
        auto ext = it->path().extension();
        if (ext != L".dmp" && ext != L".mdmp") {
            continue;
        }
        if (!is_steam_related_filename(it->path().filename().wstring())) {
            continue;
        }
        DumpFile d;
        d.path = it->path();
        std::error_code ec2;
        d.size_bytes = it->file_size(ec2);
        out.push_back(std::move(d));
    }
    return out;
}

}  // namespace stc::core::crashdumps
