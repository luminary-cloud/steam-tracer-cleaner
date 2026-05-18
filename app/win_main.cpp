// Win32 + D3D11 + Dear ImGui entry point.
// Adapted from the upstream imgui example_win32_directx11, trimmed for our use.

#include <windows.h>

#include <d3d11.h>
#include <dxgi.h>
#include <tchar.h>

#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>

#include <spdlog/spdlog.h>

#include "app/app_state.hpp"
#include "core/cleaner.hpp"
#include "core/dry_run.hpp"
#include "core/log.hpp"
#include "core/profile.hpp"
#include "core/version.hpp"
#include "platform/paths.hpp"
#include "ui/fonts.hpp"
#include "ui/icons.hpp"
#include "ui/main_window.hpp"
#include "ui/theme.hpp"

#include <cwchar>

// imgui_impl_win32.h keeps this declaration inside `#if 0` so its header doesn't drag in
// <windows.h>. Must stay in the global namespace; an anonymous-namespace forward decl gets
// internal linkage and the link fails.
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam,
                                                             LPARAM lParam);

namespace {

constexpr wchar_t kWindowClass[] = L"steam-tracer-cleaner";
constexpr wchar_t kWindowTitle[] = L"Steam Tracer Cleaner";
constexpr int kInitialWidth = 1240;
constexpr int kInitialHeight = 820;

ID3D11Device* g_device = nullptr;
ID3D11DeviceContext* g_context = nullptr;
IDXGISwapChain* g_swap_chain = nullptr;
ID3D11RenderTargetView* g_rtv = nullptr;
UINT g_resize_w = 0;
UINT g_resize_h = 0;

void create_render_target() {
    ID3D11Texture2D* back_buffer = nullptr;
    g_swap_chain->GetBuffer(0, IID_PPV_ARGS(&back_buffer));
    if (back_buffer) {
        g_device->CreateRenderTargetView(back_buffer, nullptr, &g_rtv);
        back_buffer->Release();
    }
}

void destroy_render_target() {
    if (g_rtv) {
        g_rtv->Release();
        g_rtv = nullptr;
    }
}

bool create_device(HWND hwnd) {
    DXGI_SWAP_CHAIN_DESC sd{};
    sd.BufferCount = 2;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hwnd;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT flags = 0;
#ifdef _DEBUG
    flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    const D3D_FEATURE_LEVEL levels[] = {D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0};
    D3D_FEATURE_LEVEL chosen{};
    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, flags, levels, _countof(levels),
        D3D11_SDK_VERSION, &sd, &g_swap_chain, &g_device, &chosen, &g_context);

    // Some integrated GPUs reject the debug flag silently. Retry without it.
    if (hr == DXGI_ERROR_SDK_COMPONENT_MISSING) {
        flags &= ~D3D11_CREATE_DEVICE_DEBUG;
        hr = D3D11CreateDeviceAndSwapChain(
            nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, flags, levels, _countof(levels),
            D3D11_SDK_VERSION, &sd, &g_swap_chain, &g_device, &chosen, &g_context);
    }
    if (FAILED(hr)) {
        spdlog::error("D3D11CreateDeviceAndSwapChain failed: hr=0x{:08x}", static_cast<unsigned>(hr));
        return false;
    }
    create_render_target();
    return true;
}

void destroy_device() {
    destroy_render_target();
    if (g_swap_chain) {
        g_swap_chain->Release();
        g_swap_chain = nullptr;
    }
    if (g_context) {
        g_context->Release();
        g_context = nullptr;
    }
    if (g_device) {
        g_device->Release();
        g_device = nullptr;
    }
}

LRESULT CALLBACK wnd_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wp, lp)) {
        return TRUE;
    }
    switch (msg) {
        case WM_SIZE:
            if (wp != SIZE_MINIMIZED) {
                g_resize_w = LOWORD(lp);
                g_resize_h = HIWORD(lp);
            }
            return 0;
        case WM_SYSCOMMAND:
            // Block Alt key from popping the system menu, ImGui handles its own.
            if ((wp & 0xfff0) == SC_KEYMENU) {
                return 0;
            }
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        default:
            break;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

}  // namespace

namespace {

int run_headless_clean() {
    stc::app::AppState state;
    try {
        state.initialize();
    } catch (const std::exception& e) {
        spdlog::error("AppState init failed: {}", e.what());
        return 1;
    }
    if (!state.install) {
        spdlog::error("Steam install not found, aborting scheduled clean");
        return 1;
    }

    auto profiles = stc::core::built_in_profiles();
    if (profiles.empty()) {
        return 1;
    }
    const auto& profile = profiles.front();  // Quick Clean

    stc::core::ResolveContext ctx{*state.install, state.accounts, state.libraries};
    stc::core::PlanOptions opts;
    opts.ignore = &state.ignore_list;

    auto plan = stc::core::build_plan_by_ids(profile.target_ids, ctx, opts);

    auto session = stc::core::backup::Session::create(state.backups_dir);
    stc::core::CleanOptions cleaner_opts;
    cleaner_opts.backup = session ? &*session : nullptr;
    auto result = stc::core::execute(plan, cleaner_opts);
    spdlog::info("Headless clean finished: {} ok, {} failed, {} bytes freed", result.succeeded,
                 result.failed, result.bytes_freed);
    return result.failed > 0 ? 2 : 0;
}

bool has_flag(const wchar_t* cmdline, const wchar_t* flag) {
    return cmdline && wcsstr(cmdline, flag) != nullptr;
}

}  // namespace

int APIENTRY wWinMain(HINSTANCE inst, HINSTANCE, PWSTR cmdline, int show) {
    try {
        stc::core::log::init(stc::platform::local_appdata_dir() / "steam-tracer-cleaner");
    } catch (...) {
        // Logging is best-effort. If %LOCALAPPDATA% is unavailable for some reason, fall back to
        // the executable directory.
        try {
            stc::core::log::init(stc::platform::exe_directory());
        } catch (...) {  // NOLINT(bugprone-empty-catch)
        }
    }
    spdlog::info("Starting steam-tracer-cleaner {}", stc::core::kAppVersion);

    if (has_flag(cmdline, L"--scheduled")) {
        spdlog::info("Headless mode (--scheduled)");
        int rc = run_headless_clean();
        stc::core::log::flush_all();
        return rc;
    }

    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.style = CS_CLASSDC;
    wc.lpfnWndProc = wnd_proc;
    wc.hInstance = inst;
    wc.hIcon = LoadIconW(inst, MAKEINTRESOURCEW(101));
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.lpszClassName = kWindowClass;
    RegisterClassExW(&wc);

    HWND hwnd = CreateWindowExW(0, wc.lpszClassName, kWindowTitle, WS_OVERLAPPEDWINDOW,
                                CW_USEDEFAULT, CW_USEDEFAULT, kInitialWidth, kInitialHeight,
                                nullptr, nullptr, inst, nullptr);

    if (!create_device(hwnd)) {
        destroy_device();
        UnregisterClassW(wc.lpszClassName, inst);
        return 1;
    }

    ShowWindow(hwnd, show);
    UpdateWindow(hwnd);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.IniFilename = nullptr;  // we manage layout ourselves later

    stc::ui::fonts::load();
    stc::ui::theme::apply_dark();
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_device, g_context);
    stc::ui::icons::load(g_device);

    stc::app::AppState state;
    try {
        state.initialize();
    } catch (const std::exception& e) {
        spdlog::error("AppState init failed: {}", e.what());
    }

    bool running = true;
    while (running) {
        MSG msg;
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
            if (msg.message == WM_QUIT) {
                running = false;
            }
        }
        if (!running) {
            break;
        }

        if (g_resize_w != 0 && g_resize_h != 0) {
            destroy_render_target();
            g_swap_chain->ResizeBuffers(0, g_resize_w, g_resize_h, DXGI_FORMAT_UNKNOWN, 0);
            g_resize_w = g_resize_h = 0;
            create_render_target();
        }

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        stc::ui::draw_main_window(state);

        ImGui::Render();
        const float bg[4] = {0.07F, 0.08F, 0.09F, 1.0F};
        g_context->OMSetRenderTargets(1, &g_rtv, nullptr);
        g_context->ClearRenderTargetView(g_rtv, bg);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        g_swap_chain->Present(1, 0);
    }

    stc::ui::icons::shutdown();
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    destroy_device();
    DestroyWindow(hwnd);
    UnregisterClassW(wc.lpszClassName, inst);

    spdlog::info("Shutdown clean");
    stc::core::log::flush_all();
    return 0;
}
