#pragma once

namespace stc::ui::widgets {

// Geometry shared with the window procedure in app/win_main.cpp: its
// WM_NCHITTEST carves out the same caption strip and button block this widget
// draws, so dragging and the control buttons line up. Logical pixels at 100%
// DPI; wnd_proc scales these by GetDpiForWindow / 96.
inline constexpr float kTitleBarHeight  = 34.0F;
inline constexpr float kCaptionBtnWidth = 46.0F;
inline constexpr int   kCaptionBtnCount = 3;   // minimize, maximize, close
inline constexpr float kResizeBorder    = 6.0F;

// Draws the custom title bar across the top of the root window. Gets the window
// handle from ImGui's main viewport, so it needs no AppState.
void draw_title_bar();

}  // namespace stc::ui::widgets
