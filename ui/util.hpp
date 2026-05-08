#pragma once

#include <windows.h>

#include <imgui.h>

#include <string>
#include <string_view>

namespace stc::ui {

inline std::string to_utf8(std::wstring_view s) {
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

inline std::wstring from_utf8(std::string_view s) {
    if (s.empty()) {
        return {};
    }
    int n = MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), nullptr, 0);
    std::wstring out(static_cast<std::size_t>(n), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), out.data(), n);
    return out;
}

// Shows `text` as a tooltip if the previous widget is hovered. Call right after the widget you
// want to annotate.
inline void hover_tooltip(const char* text) {
    if (text != nullptr && text[0] != '\0' && ImGui::IsItemHovered()) {
        ImGui::SetTooltip("%s", text);
    }
}

inline std::string format_bytes(std::uint64_t bytes) {
    constexpr std::uint64_t kKiB = 1024ULL;
    constexpr std::uint64_t kMiB = 1024ULL * 1024ULL;
    constexpr std::uint64_t kGiB = 1024ULL * 1024ULL * 1024ULL;
    char buf[64];
    if (bytes >= kGiB) {
        std::snprintf(buf, sizeof(buf), "%.2f GiB", static_cast<double>(bytes) / kGiB);
    } else if (bytes >= kMiB) {
        std::snprintf(buf, sizeof(buf), "%.1f MiB", static_cast<double>(bytes) / kMiB);
    } else if (bytes >= kKiB) {
        std::snprintf(buf, sizeof(buf), "%.0f KiB", static_cast<double>(bytes) / kKiB);
    } else {
        std::snprintf(buf, sizeof(buf), "%llu B", static_cast<unsigned long long>(bytes));
    }
    return buf;
}

}  // namespace stc::ui
