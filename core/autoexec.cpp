#include "core/autoexec.hpp"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <system_error>

#include "core/appid_map.hpp"
#include "platform/fs.hpp"

namespace stc::core::autoexec {
namespace {

namespace fs_std = std::filesystem;

std::wstring timestamp_suffix() {
    auto now = std::chrono::system_clock::now();
    auto t = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
    gmtime_s(&tm, &t);
    std::wstringstream ss;
    ss << std::put_time(&tm, L"%Y%m%dT%H%M%SZ");
    return ss.str();
}

InstallResult copy_with_backup(const fs_std::path& src, const fs_std::path& dst) {
    InstallResult r;
    r.target_path = dst;

    std::error_code ec;
    if (!fs_std::exists(src, ec)) {
        r.error = L"Source file does not exist";
        return r;
    }
    fs_std::create_directories(dst.parent_path(), ec);

    if (fs_std::exists(dst, ec)) {
        auto bak = dst;
        bak += L".bak." + timestamp_suffix();
        fs_std::copy_file(dst, bak, fs_std::copy_options::overwrite_existing, ec);
        if (ec) {
            r.error = L"Backup of existing file failed";
            return r;
        }
        r.backup_path = bak;
    }

    fs_std::copy_file(src, dst, fs_std::copy_options::overwrite_existing, ec);
    if (ec) {
        r.error = std::wstring{L"copy_file failed: "} +
                  std::wstring{ec.message().begin(), ec.message().end()};
        return r;
    }
    r.ok = true;
    return r;
}

}  // namespace

InstallResult install_autoexec(const std::vector<fs_std::path>& libraries, std::uint32_t appid,
                               const fs_std::path& source_cfg, const std::wstring& dest_filename) {
    InstallResult r;
    const auto* game = stc::core::appid::find_by_appid(appid);
    if (!game) {
        r.error = L"Unknown AppID";
        return r;
    }

    for (const auto& lib : libraries) {
        auto game_dir = lib / "common" / std::wstring{game->install_folder};
        std::error_code ec;
        if (!fs_std::is_directory(game_dir, ec)) {
            continue;
        }
        // mod_folder may contain forward slashes ("game/csgo"). Resolve as a relative path.
        fs_std::path mod_path = std::wstring{game->mod_folder};
        auto cfg_path = game_dir / mod_path / "cfg" / dest_filename;
        return copy_with_backup(source_cfg, cfg_path);
    }
    r.error = L"Game install folder not found in any Steam library";
    return r;
}

VideoConfigResult install_video_config(const stc::core::steam::InstallInfo& /*install*/,
                                       const std::vector<stc::core::steam::AccountInfo>& accounts,
                                       const std::vector<std::uint32_t>& account_ids,
                                       std::uint32_t appid,
                                       const fs_std::path& source_video_txt) {
    VideoConfigResult out;
    if (!fs_std::exists(source_video_txt)) {
        out.ok = false;
        return out;
    }

    for (std::uint32_t account_id : account_ids) {
        auto match = std::find_if(accounts.begin(), accounts.end(), [&](const auto& a) {
            return a.account_id == account_id;
        });
        if (match == accounts.end()) {
            continue;
        }
        // Filename matches what CS2 itself writes.
        std::wstring filename = appid == 730 ? L"cs2_video.txt" : L"video.txt";
        auto dst =
            match->userdata_path / std::to_wstring(appid) / "local" / "cfg" / filename;
        auto r = copy_with_backup(source_video_txt, dst);
        if (!r.ok) {
            out.ok = false;
        }
        out.per_account.push_back(std::move(r));
    }
    return out;
}

}  // namespace stc::core::autoexec
