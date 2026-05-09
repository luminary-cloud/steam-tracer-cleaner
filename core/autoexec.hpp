#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

#include "core/steam_paths.hpp"

namespace stc::core::autoexec {

struct InstallResult {
    bool ok = false;
    std::filesystem::path target_path;
    std::filesystem::path backup_path;     // empty if no existing file was overwritten
    std::wstring error;
};

// Locates the install folder for `appid` across every known Steam library and copies `source_cfg`
// to its `<install>/<mod>/cfg/autoexec.cfg`. If a file already exists at the destination, it is
// renamed to `autoexec.cfg.bak.<timestamp>` before overwrite.
InstallResult install_autoexec(const std::vector<std::filesystem::path>& libraries,
                               std::uint32_t appid, const std::filesystem::path& source_cfg,
                               const std::wstring& dest_filename = L"autoexec.cfg");

struct VideoConfigResult {
    bool ok = true;
    std::vector<InstallResult> per_account;  // one entry per attempted account
};

// Copies `source_video_txt` to each selected account's `userdata/<id>/<appid>/local/cfg/cs2_video.txt`.
// `appid` defaults to 730 (CS2). Existing files are backed up alongside as `.bak.<timestamp>`.
VideoConfigResult install_video_config(const stc::core::steam::InstallInfo& install,
                                       const std::vector<stc::core::steam::AccountInfo>& accounts,
                                       const std::vector<std::uint32_t>& account_ids,
                                       std::uint32_t appid,
                                       const std::filesystem::path& source_video_txt);

}  // namespace stc::core::autoexec
