#pragma once

#include <filesystem>

namespace stc::platform {

std::filesystem::path exe_path();
std::filesystem::path exe_directory();
std::filesystem::path appdata_dir();
std::filesystem::path local_appdata_dir();
std::filesystem::path program_files_x86();
std::filesystem::path windows_dir();
std::filesystem::path temp_dir();

}  // namespace stc::platform
