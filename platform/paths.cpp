#include "platform/paths.hpp"

#include <windows.h>

#include <shlobj.h>

#include <array>
#include <stdexcept>

namespace stc::platform {
namespace {

std::filesystem::path known_folder(REFKNOWNFOLDERID id) {
    PWSTR raw = nullptr;
    HRESULT hr = SHGetKnownFolderPath(id, KF_FLAG_DEFAULT, nullptr, &raw);
    if (FAILED(hr) || raw == nullptr) {
        if (raw) {
            CoTaskMemFree(raw);
        }
        throw std::runtime_error("SHGetKnownFolderPath failed");
    }
    std::filesystem::path result{raw};
    CoTaskMemFree(raw);
    return result;
}

}  // namespace

std::filesystem::path exe_path() {
    std::array<wchar_t, 32768> buffer{};
    DWORD len = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    if (len == 0 || len == buffer.size()) {
        throw std::runtime_error("GetModuleFileNameW failed");
    }
    return std::filesystem::path{std::wstring_view{buffer.data(), len}};
}

std::filesystem::path exe_directory() {
    return exe_path().parent_path();
}

std::filesystem::path appdata_dir() {
    return known_folder(FOLDERID_RoamingAppData);
}

std::filesystem::path local_appdata_dir() {
    return known_folder(FOLDERID_LocalAppData);
}

std::filesystem::path program_files_x86() {
    return known_folder(FOLDERID_ProgramFilesX86);
}

std::filesystem::path windows_dir() {
    std::array<wchar_t, MAX_PATH + 1> buffer{};
    UINT len = GetWindowsDirectoryW(buffer.data(), static_cast<UINT>(buffer.size()));
    if (len == 0) {
        throw std::runtime_error("GetWindowsDirectoryW failed");
    }
    return std::filesystem::path{std::wstring_view{buffer.data(), len}};
}

std::filesystem::path temp_dir() {
    std::array<wchar_t, MAX_PATH + 2> buffer{};
    DWORD len = GetTempPathW(static_cast<DWORD>(buffer.size()), buffer.data());
    if (len == 0) {
        throw std::runtime_error("GetTempPathW failed");
    }
    return std::filesystem::path{std::wstring_view{buffer.data(), len}};
}

}  // namespace stc::platform
