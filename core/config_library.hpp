#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace stc::core::config_library {

enum class ConfigKind { Autoexec, Video };

struct LibraryEntry {
    std::filesystem::path path;
    std::string filename;
};

std::filesystem::path import_config(const std::filesystem::path& library_root, ConfigKind kind,
                                    const std::filesystem::path& source);

std::vector<LibraryEntry> list_configs(const std::filesystem::path& library_root, ConfigKind kind);

bool remove_config(const std::filesystem::path& entry_path);

}  // namespace stc::core::config_library
