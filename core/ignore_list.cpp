#include "core/ignore_list.hpp"

#include <windows.h>

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <cwctype>
#include <fstream>
#include <sstream>
#include <system_error>

namespace stc::core {
namespace {

bool starts_with_ci(std::wstring_view haystack, std::wstring_view needle) {
    if (needle.size() > haystack.size()) {
        return false;
    }
    for (std::size_t i = 0; i < needle.size(); ++i) {
        if (towlower(haystack[i]) != towlower(needle[i])) {
            return false;
        }
    }
    return true;
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

std::string w_to_utf8(const std::wstring& s) {
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

}  // namespace

bool IgnoreList::preserves_account(std::wstring_view steamid64) const {
    if (steamid64.empty()) {
        return false;
    }
    return std::any_of(preserved_account_ids.begin(), preserved_account_ids.end(),
                       [&](const std::wstring& id) { return id == steamid64; });
}

bool IgnoreList::preserves_path(std::wstring_view path) const {
    return std::any_of(preserved_paths.begin(), preserved_paths.end(),
                       [&](const std::wstring& p) { return starts_with_ci(path, p); });
}

bool IgnoreList::preserves_registry(std::wstring_view full_key, std::wstring_view value_name) const {
    for (const auto& entry : preserved_registry_values) {
        // entry can be either a key prefix ("HKCU\\...\\Users") or a key + value "::" value.
        auto sep = entry.find(L"::");
        if (sep == std::wstring::npos) {
            if (starts_with_ci(full_key, entry)) {
                return true;
            }
        } else {
            std::wstring_view key{entry.data(), sep};
            std::wstring_view name{entry.data() + sep + 2, entry.size() - sep - 2};
            if (starts_with_ci(full_key, key) && (name.empty() || name == value_name)) {
                return true;
            }
        }
    }
    return false;
}

std::optional<IgnoreList> load_ignore_list(const std::filesystem::path& json_path) {
    std::ifstream f(json_path);
    if (!f) {
        return std::nullopt;
    }
    nlohmann::json j;
    try {
        f >> j;
    } catch (const std::exception& e) {
        spdlog::warn("ignore.json parse failed: {}", e.what());
        return std::nullopt;
    }

    IgnoreList list;
    if (j.contains("preserved_account_ids")) {
        for (const auto& id : j.at("preserved_account_ids")) {
            list.preserved_account_ids.push_back(utf8_to_w(id.get<std::string>()));
        }
    }
    if (j.contains("preserved_ssfn_files")) {
        list.preserve_all_ssfn = j.at("preserved_ssfn_files").get<bool>();
    }
    if (j.contains("preserved_paths")) {
        for (const auto& p : j.at("preserved_paths")) {
            list.preserved_paths.push_back(utf8_to_w(p.get<std::string>()));
        }
    }
    if (j.contains("preserved_registry_values")) {
        for (const auto& p : j.at("preserved_registry_values")) {
            list.preserved_registry_values.push_back(utf8_to_w(p.get<std::string>()));
        }
    }
    return list;
}

bool save_ignore_list(const IgnoreList& list, const std::filesystem::path& json_path) {
    nlohmann::json j;
    j["preserved_account_ids"] = nlohmann::json::array();
    for (const auto& id : list.preserved_account_ids) {
        j["preserved_account_ids"].push_back(w_to_utf8(id));
    }
    j["preserved_ssfn_files"] = list.preserve_all_ssfn;
    j["preserved_paths"] = nlohmann::json::array();
    for (const auto& p : list.preserved_paths) {
        j["preserved_paths"].push_back(w_to_utf8(p));
    }
    j["preserved_registry_values"] = nlohmann::json::array();
    for (const auto& p : list.preserved_registry_values) {
        j["preserved_registry_values"].push_back(w_to_utf8(p));
    }

    std::error_code ec;
    std::filesystem::create_directories(json_path.parent_path(), ec);
    std::ofstream f(json_path, std::ios::trunc);
    if (!f) {
        return false;
    }
    f << j.dump(4);
    return f.good();
}

IgnoreList default_ignore_list() {
    IgnoreList list;
    list.preserve_all_ssfn = false;
    return list;
}

}  // namespace stc::core
