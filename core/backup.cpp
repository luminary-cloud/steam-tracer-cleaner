#include "core/backup.hpp"

#include <windows.h>

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <chrono>
#include <ctime>
#include <cwctype>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <system_error>
#include <vector>

#include "platform/fs.hpp"

namespace stc::core::backup {
namespace {

namespace fs_std = std::filesystem;

std::wstring make_timestamp() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::tm utc{};
    gmtime_s(&utc, &time);
    std::wstringstream ss;
    ss << std::put_time(&utc, L"%Y%m%dT%H%M%SZ");
    return ss.str();
}

std::wstring sanitize(std::wstring_view in) {
    std::wstring out;
    out.reserve(in.size());
    for (wchar_t ch : in) {
        if (iswalnum(ch) || ch == L'-' || ch == L'_' || ch == L'.') {
            out.push_back(ch);
        } else {
            out.push_back(L'_');
        }
    }
    return out;
}

std::string utf8(const std::wstring& s) {
    if (s.empty()) {
        return {};
    }
    int n = WideCharToMultiByte(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), nullptr, 0, nullptr,
                                nullptr);
    std::string out(static_cast<std::size_t>(n), '\0');
    WideCharToMultiByte(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), out.data(), n, nullptr,
                        nullptr);
    return out;
}

std::wstring utf8_to_w(const std::string& s) {
    if (s.empty()) {
        return {};
    }
    int n = MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), nullptr, 0);
    std::wstring out(static_cast<std::size_t>(n), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), out.data(), n);
    return out;
}

bool run_reg_command(const std::wstring& cmdline) {
    std::wstring mut = cmdline;  // CreateProcess wants writable
    STARTUPINFOW si{};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    PROCESS_INFORMATION pi{};
    if (!CreateProcessW(nullptr, mut.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr,
                        nullptr, &si, &pi)) {
        spdlog::warn("CreateProcessW failed for reg.exe: {}", GetLastError());
        return false;
    }
    WaitForSingleObject(pi.hProcess, 30000);
    DWORD code = 0;
    GetExitCodeProcess(pi.hProcess, &code);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return code == 0;
}

std::filesystem::path file_mirror_path(const std::filesystem::path& backup_dir,
                                       const std::filesystem::path& src) {
    auto abs = src.lexically_normal();
    auto root_name = abs.root_name().wstring();  // "C:" -> "C"
    if (!root_name.empty() && root_name.back() == L':') {
        root_name.pop_back();
    }
    if (root_name.empty()) {
        root_name = L"_";
    }
    auto rel = abs.relative_path();
    return backup_dir / "files" / root_name / rel;
}

}  // namespace

Session::Session(std::filesystem::path d, std::wstring ts) : dir_(std::move(d)), timestamp_(std::move(ts)) {
    manifest_.timestamp = timestamp_;
}

std::optional<Session> Session::create(const std::filesystem::path& backups_root) {
    auto ts = make_timestamp();
    auto dir = backups_root / ts;
    std::error_code ec;
    fs_std::create_directories(dir / "files", ec);
    fs_std::create_directories(dir / "registry", ec);
    if (ec) {
        spdlog::error("create_directories({}) failed: {}", dir.string(), ec.message());
        return std::nullopt;
    }
    return Session{dir, ts};
}

bool Session::record_file(const fs_std::path& src) {
    if (!stc::platform::fsx::exists(src)) {
        return true;
    }
    auto dst = file_mirror_path(dir_, src);
    return stc::platform::fsx::copy_file(src, dst);
}

bool Session::record_directory(const fs_std::path& src) {
    if (!stc::platform::fsx::exists(src)) {
        return true;
    }
    auto dst = file_mirror_path(dir_, src);
    return stc::platform::fsx::copy_tree(src, dst);
}

bool Session::record_registry_key(std::wstring_view full_key) {
    auto safe = sanitize(full_key) + L".reg";
    auto out = dir_ / "registry" / safe;
    std::wstring cmd = L"reg.exe export \"" + std::wstring{full_key} + L"\" \"" + out.wstring() +
                       L"\" /y";
    return run_reg_command(cmd);
}

bool Session::record_registry_value(std::wstring_view full_key,
                                    std::wstring_view /*value_name*/) {
    // reg.exe export covers the whole key. Granular per-value export is not a built-in reg.exe
    // feature, so we save the key and accept that restore will overwrite siblings too.
    return record_registry_key(full_key);
}

bool Session::record_vdf(const fs_std::path& vdf_path) { return record_file(vdf_path); }

bool Session::finalize() {
    // The nlohmann_json + utf8 conversions throw on bad UTF-16 in op.target / op.value_name.
    // Saw it once with a path containing a stray lone surrogate, easier to swallow it here than
    // propagate a half-finalized backup to the caller.
    try {
        nlohmann::json j;
        j["timestamp"] = utf8(timestamp_);
        j["bytes_saved"] = manifest_.bytes_saved;

        nlohmann::json ops = nlohmann::json::array();
        for (const auto& op : manifest_.operations) {
            nlohmann::json oj;
            oj["kind"] = static_cast<int>(op.kind);
            oj["target"] = utf8(op.target);
            oj["value_name"] = utf8(op.value_name);
            oj["account_steamid64"] = utf8(op.account_steamid64);
            oj["size_bytes"] = op.size_bytes;
            ops.push_back(std::move(oj));
        }
        j["operations"] = std::move(ops);

        std::ofstream f(dir_ / "manifest.json", std::ios::trunc);
        if (!f) {
            return false;
        }
        f << j.dump(2);
        return f.good();
    } catch (const std::exception& e) {
        spdlog::warn("backup manifest serialization failed: {}", e.what());
        return false;
    }
}

std::vector<Entry> list_backups(const fs_std::path& backups_root) {
    std::vector<Entry> entries;
    std::error_code ec;
    if (!fs_std::is_directory(backups_root, ec)) {
        return entries;
    }
    for (auto it = fs_std::directory_iterator(backups_root, ec); !ec && it != fs_std::end(it);
         it.increment(ec)) {
        if (!it->is_directory(ec)) {
            continue;
        }
        Entry e;
        e.dir = it->path();
        e.timestamp = it->path().filename().wstring();
        e.size_bytes = stc::platform::fsx::size_recursive(e.dir);

        auto manifest = it->path() / "manifest.json";
        std::ifstream f(manifest);
        if (f) {
            try {
                nlohmann::json j;
                f >> j;
                e.op_count = j.value("operations", nlohmann::json::array()).size();
            } catch (...) {
                // ignore; the entry is still listable
            }
        }
        entries.push_back(std::move(e));
    }

    std::sort(entries.begin(), entries.end(),
              [](const Entry& a, const Entry& b) { return a.timestamp > b.timestamp; });
    return entries;
}

bool restore(const Entry& e) {
    bool all_ok = true;

    // Restore files: walk <backup>/files/<drive>/... and copy back.
    auto files_root = e.dir / "files";
    std::error_code ec;
    if (fs_std::is_directory(files_root, ec)) {
        for (auto it = fs_std::recursive_directory_iterator(files_root, ec);
             !ec && it != fs_std::recursive_directory_iterator(); it.increment(ec)) {
            if (!it->is_regular_file(ec)) {
                continue;
            }
            auto rel = fs_std::relative(it->path(), files_root, ec);
            if (rel.empty()) {
                continue;
            }
            auto first = rel.begin();
            if (first == rel.end()) {
                continue;
            }
            auto drive = first->wstring() + L":";
            fs_std::path remainder;
            ++first;
            for (auto i = first; i != rel.end(); ++i) {
                remainder /= *i;
            }
            auto dst = fs_std::path{drive} / remainder;
            if (!stc::platform::fsx::copy_file(it->path(), dst)) {
                all_ok = false;
            }
        }
    }

    // Restore registry: import every .reg under <backup>/registry/.
    auto reg_root = e.dir / "registry";
    if (fs_std::is_directory(reg_root, ec)) {
        for (auto it = fs_std::directory_iterator(reg_root, ec); !ec && it != fs_std::end(it);
             it.increment(ec)) {
            if (!it->is_regular_file(ec)) {
                continue;
            }
            std::wstring cmd = L"reg.exe import \"" + it->path().wstring() + L"\"";
            if (!run_reg_command(cmd)) {
                all_ok = false;
            }
        }
    }
    return all_ok;
}

bool remove_backup(const Entry& e) {
    return stc::platform::fsx::delete_directory_recursive(e.dir);
}

std::size_t prune_backups(const fs_std::path& backups_root, std::size_t keep) {
    auto entries = list_backups(backups_root);  // already sorted newest-first
    std::size_t removed = 0;
    for (std::size_t i = keep; i < entries.size(); ++i) {
        if (remove_backup(entries[i])) {
            ++removed;
        }
    }
    return removed;
}

}  // namespace stc::core::backup
