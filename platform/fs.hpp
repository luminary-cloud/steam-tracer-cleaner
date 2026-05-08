#pragma once

#include <cstdint>
#include <filesystem>

namespace stc::platform::fsx {

bool exists(const std::filesystem::path& p);

// Sum of file sizes under `p`, recursively. Returns 0 if `p` is missing or unreadable.
std::uintmax_t size_recursive(const std::filesystem::path& p);

// Counts every regular file under `p`, recursively.
std::uintmax_t file_count_recursive(const std::filesystem::path& p);

// Tries DeleteFileW. On sharing violations, retries with a CreateFileW + FILE_FLAG_DELETE_ON_CLOSE
// dance. Returns true if the file is gone (or never existed).
bool delete_file(const std::filesystem::path& p);

// Walks `p` bottom-up, deleting files and then directories. Best-effort, does not throw.
bool delete_directory_recursive(const std::filesystem::path& p);

// Copies `from` to `to`. Creates intermediate directories. Overwrites existing target. Returns
// false on failure.
bool copy_file(const std::filesystem::path& from, const std::filesystem::path& to);

// Recursive copy. Mirror semantics: target tree gets the same files as source.
bool copy_tree(const std::filesystem::path& from, const std::filesystem::path& to);

}  // namespace stc::platform::fsx
