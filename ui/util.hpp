#pragma once

#include <windows.h>

#include <shellapi.h>

#include <imgui.h>

#include <cstdarg>
#include <string>
#include <string_view>

namespace stc::ui {

// Gap below each screen's body, between the content and the status bar. draw_content() draws it with
// zero leading spacing, so this is the exact distance; the Cleaner screen's self-correcting layout
// targets it directly to sit its button row this far above the footer without an outer scrollbar.
inline constexpr float kContentPaddingBottom = 4.0F;

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

inline bool begin_styled_modal(const char* name, float width = 420.0F) {
    ImGui::SetNextWindowSize(ImVec2(width, 0), ImGuiCond_Always);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16, 14));
    bool open = ImGui::BeginPopupModal(
        name, nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings);
    if (!open) {
        ImGui::PopStyleVar();
    }
    return open;
}

inline void end_styled_modal() {
    ImGui::EndPopup();
    ImGui::PopStyleVar();
}

// Shows `text` as a tooltip if the previous widget is hovered. Call right after the widget you
// want to annotate.
inline void hover_tooltip(const char* text) {
    if (text != nullptr && text[0] != '\0' && ImGui::IsItemHovered()) {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 6));
        ImGui::SetTooltip("%s", text);
        ImGui::PopStyleVar();
    }
}

// Drop-in for ImGui::SetTooltip that restores the inner WindowPadding the global
// (0, 0) theme strips, so tooltip text isn't flush against the border.
inline void set_tooltip(const char* fmt, ...) IM_FMTARGS(1) {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 6));
    va_list args;
    va_start(args, fmt);
    ImGui::SetTooltipV(fmt, args);
    va_end(args);
    ImGui::PopStyleVar();
}

inline void open_url(std::wstring_view url) {
    std::wstring s{url};
    ShellExecuteW(nullptr, L"open", s.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
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
