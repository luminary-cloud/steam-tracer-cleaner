#include "ui/fonts.hpp"

#include <windows.h>

#include <filesystem>
#include <string>

namespace stc::ui::fonts {
namespace {

ImFont* g_body = nullptr;
ImFont* g_title = nullptr;

std::filesystem::path windows_fonts_dir() {
    wchar_t buf[MAX_PATH] = {};
    UINT n = GetWindowsDirectoryW(buf, MAX_PATH);
    if (n == 0) {
        return L"C:\\Windows\\Fonts";
    }
    return std::filesystem::path{std::wstring{buf, n}} / L"Fonts";
}

}  // namespace

void load() {
    ImGuiIO& io = ImGui::GetIO();

    ImFontConfig cfg;
    cfg.PixelSnapH = false;
    cfg.OversampleH = 5;
    cfg.OversampleV = 5;
    cfg.RasterizerMultiply = 1.20F;

    static const ImWchar ranges[] = {
        0x0020, 0x00FF,   // Latin
        0x2000, 0x206F,   // General punctuation
        0xE000, 0xF8FF,   // Private Use Area (icons)
        0,
    };
    cfg.GlyphRanges = ranges;

    auto fonts = windows_fonts_dir();
    auto regular = (fonts / L"segoeui.ttf").string();
    auto bold = (fonts / L"segoeuib.ttf").string();

    g_body = io.Fonts->AddFontFromFileTTF(regular.c_str(), 16.0F, &cfg);
    g_title = io.Fonts->AddFontFromFileTTF(bold.c_str(), 17.0F, &cfg);

    if (!g_body) {
        g_body = io.Fonts->AddFontDefault();
    }
    if (!g_title) {
        g_title = g_body;
    }
}

ImFont* body() { return g_body; }
ImFont* title() { return g_title; }

}  // namespace stc::ui::fonts
