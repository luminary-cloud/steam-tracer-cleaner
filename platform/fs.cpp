#include "platform/fs.hpp"

#include <windows.h>

#include <spdlog/spdlog.h>

#include <system_error>

namespace stc::platform::fsx {
namespace {

namespace fs_std = std::filesystem;

bool delete_via_share_handle(const fs_std::path& p) {
    HANDLE h = CreateFileW(p.c_str(), DELETE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                           nullptr, OPEN_EXISTING,
                           FILE_FLAG_DELETE_ON_CLOSE | FILE_FLAG_BACKUP_SEMANTICS, nullptr);
    if (h == INVALID_HANDLE_VALUE) {
        return false;
    }
    CloseHandle(h);
    // The actual delete happens on close. Verify.
    return GetFileAttributesW(p.c_str()) == INVALID_FILE_ATTRIBUTES;
}

}  // namespace

bool exists(const fs_std::path& p) {
    std::error_code ec;
    return fs_std::exists(p, ec);
}

std::uintmax_t size_recursive(const fs_std::path& p) {
    std::error_code ec;
    if (!fs_std::exists(p, ec)) {
        return 0;
    }
    if (fs_std::is_regular_file(p, ec)) {
        auto sz = fs_std::file_size(p, ec);
        return ec ? 0 : sz;
    }
    std::uintmax_t total = 0;
    for (auto it = fs_std::recursive_directory_iterator(p, fs_std::directory_options::skip_permission_denied, ec);
         !ec && it != fs_std::recursive_directory_iterator(); it.increment(ec)) {
        if (ec) {
            ec.clear();
            continue;
        }
        std::error_code inner;
        if (it->is_regular_file(inner)) {
            auto sz = it->file_size(inner);
            if (!inner) {
                total += sz;
            }
        }
    }
    return total;
}

std::uintmax_t file_count_recursive(const fs_std::path& p) {
    std::error_code ec;
    if (!fs_std::exists(p, ec)) {
        return 0;
    }
    if (fs_std::is_regular_file(p, ec)) {
        return 1;
    }
    std::uintmax_t count = 0;
    for (auto it = fs_std::recursive_directory_iterator(p, fs_std::directory_options::skip_permission_denied, ec);
         !ec && it != fs_std::recursive_directory_iterator(); it.increment(ec)) {
        if (ec) {
            ec.clear();
            continue;
        }
        std::error_code inner;
        if (it->is_regular_file(inner)) {
            ++count;
        }
    }
    return count;
}

bool delete_file(const fs_std::path& p) {
    std::error_code probe;
    if (!fs_std::exists(p, probe)) {
        return true;
    }
    // Drop read-only / system / hidden attributes that block DeleteFileW.
    DWORD attrs = GetFileAttributesW(p.c_str());
    if (attrs != INVALID_FILE_ATTRIBUTES && (attrs & (FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_HIDDEN |
                                                       FILE_ATTRIBUTE_SYSTEM)) != 0) {
        SetFileAttributesW(p.c_str(), attrs & ~(FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_HIDDEN |
                                                FILE_ATTRIBUTE_SYSTEM));
    }
    if (DeleteFileW(p.c_str())) {
        return true;
    }
    DWORD err = GetLastError();
    if (err == ERROR_FILE_NOT_FOUND || err == ERROR_PATH_NOT_FOUND) {
        return true;
    }
    if (err == ERROR_SHARING_VIOLATION || err == ERROR_LOCK_VIOLATION ||
        err == ERROR_ACCESS_DENIED) {
        if (delete_via_share_handle(p)) {
            return true;
        }
    }
    spdlog::warn("DeleteFileW({}) failed: {}", p.string(), err);
    return false;
}

bool delete_directory_recursive(const fs_std::path& p) {
    std::error_code ec;
    if (!fs_std::exists(p, ec)) {
        return true;
    }
    if (!fs_std::is_directory(p, ec)) {
        return delete_file(p);
    }

    bool all_ok = true;
    for (auto it = fs_std::directory_iterator(p, fs_std::directory_options::skip_permission_denied, ec);
         !ec && it != fs_std::directory_iterator(); it.increment(ec)) {
        if (ec) {
            ec.clear();
            continue;
        }
        std::error_code inner;
        if (it->is_directory(inner) && !it->is_symlink(inner)) {
            if (!delete_directory_recursive(it->path())) {
                all_ok = false;
            }
        } else {
            if (!delete_file(it->path())) {
                all_ok = false;
            }
        }
    }
    if (!RemoveDirectoryW(p.c_str())) {
        DWORD err = GetLastError();
        if (err != ERROR_FILE_NOT_FOUND && err != ERROR_PATH_NOT_FOUND) {
            spdlog::warn("RemoveDirectoryW({}) failed: {}", p.string(), err);
            all_ok = false;
        }
    }
    return all_ok;
}

bool copy_file(const fs_std::path& from, const fs_std::path& to) {
    std::error_code ec;
    fs_std::create_directories(to.parent_path(), ec);
    return CopyFileW(from.c_str(), to.c_str(), FALSE) != 0;
}

bool copy_tree(const fs_std::path& from, const fs_std::path& to) {
    std::error_code ec;
    if (!fs_std::exists(from, ec)) {
        return false;
    }
    if (fs_std::is_regular_file(from, ec)) {
        return ::stc::platform::fsx::copy_file(from, to);
    }
    fs_std::create_directories(to, ec);
    bool all_ok = true;
    for (auto it = fs_std::recursive_directory_iterator(from, fs_std::directory_options::skip_permission_denied, ec);
         !ec && it != fs_std::recursive_directory_iterator(); it.increment(ec)) {
        if (ec) {
            ec.clear();
            continue;
        }
        std::error_code inner;
        auto rel = fs_std::relative(it->path(), from, inner);
        auto dst = to / rel;
        if (it->is_directory(inner)) {
            fs_std::create_directories(dst, inner);
        } else if (it->is_regular_file(inner)) {
            if (!::stc::platform::fsx::copy_file(it->path(), dst)) {
                all_ok = false;
            }
        }
    }
    return all_ok;
}

}  // namespace stc::platform::fsx
