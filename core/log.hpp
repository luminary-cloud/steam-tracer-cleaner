#pragma once

#include <filesystem>

namespace stc::core::log {

// Initializes spdlog with a rotating file sink in `dir/logs/<name>.log` plus a debugger sink.
// Safe to call once at startup. Subsequent calls are no-ops.
void init(const std::filesystem::path& dir, const char* name = "steam-tracer-cleaner");

void flush_all();

}  // namespace stc::core::log
