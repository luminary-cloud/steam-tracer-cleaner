#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include "core/operation.hpp"

namespace stc::core::backup {

struct Manifest {
    std::wstring timestamp;                // UTC, "20260508T153000Z" style
    std::vector<Operation> operations;     // recorded operations, in apply order
    std::uint64_t bytes_saved = 0;
};

class Session {
public:
    static std::optional<Session> create(const std::filesystem::path& backups_root);

    Session(const Session&) = delete;
    Session& operator=(const Session&) = delete;
    Session(Session&&) noexcept = default;
    Session& operator=(Session&&) noexcept = default;

    const std::filesystem::path& dir() const noexcept { return dir_; }
    const std::wstring& timestamp() const noexcept { return timestamp_; }

    // Mirrors `src` (file or directory) into the backup tree under a path that mirrors the original
    // location. Returns true on success.
    bool record_file(const std::filesystem::path& src);
    bool record_directory(const std::filesystem::path& src);

    // Exports a registry key (and subtree) by shelling out to reg.exe. Saved to
    // <backup>/registry/<safe-name>.reg.
    bool record_registry_key(std::wstring_view full_key);

    // Records a registry value for ClearRegistryValue / DeleteRegistryValue. The reg.exe export is
    // smaller-grain than a full subtree.
    bool record_registry_value(std::wstring_view full_key, std::wstring_view value_name);

    // Records the raw text contents of a VDF file before in-place edits.
    bool record_vdf(const std::filesystem::path& vdf_path);

    void note_op(Operation op) { manifest_.operations.push_back(std::move(op)); }

    bool finalize();  // writes manifest.json

private:
    explicit Session(std::filesystem::path d, std::wstring ts);
    std::filesystem::path dir_;
    std::wstring timestamp_;
    Manifest manifest_;
};

struct Entry {
    std::filesystem::path dir;
    std::wstring timestamp;
    std::uint64_t size_bytes = 0;
    std::uint64_t op_count = 0;
};

std::vector<Entry> list_backups(const std::filesystem::path& backups_root);

bool restore(const Entry& e);

bool remove_backup(const Entry& e);

// Removes the oldest backups so at most `keep` remain. Returns the number of backups removed.
// `keep == 0` means remove everything.
std::size_t prune_backups(const std::filesystem::path& backups_root, std::size_t keep);

}  // namespace stc::core::backup
