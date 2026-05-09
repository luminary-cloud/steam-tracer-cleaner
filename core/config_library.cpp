#include "core/config_library.hpp"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <system_error>

namespace stc::core::config_library {
namespace {

namespace fs = std::filesystem;

fs::path kind_dir(const fs::path& root, ConfigKind kind) {
    return root / (kind == ConfigKind::Autoexec ? "autoexec" : "video");
}

fs::path deduplicate(const fs::path& dir, const fs::path& name) {
    auto candidate = dir / name;
    if (!fs::exists(candidate)) {
        return candidate;
    }
    auto stem = name.stem().wstring();
    auto ext = name.extension().wstring();
    for (int i = 2; i < 1000; ++i) {
        candidate = dir / (stem + L"_" + std::to_wstring(i) + ext);
        if (!fs::exists(candidate)) {
            return candidate;
        }
    }
    return {};
}

}  // namespace

fs::path import_config(const fs::path& library_root, ConfigKind kind, const fs::path& source) {
    std::error_code ec;
    if (!fs::exists(source, ec)) {
        return {};
    }

    auto dir = kind_dir(library_root, kind);
    fs::create_directories(dir, ec);

    auto dst = deduplicate(dir, source.filename());
    if (dst.empty()) {
        return {};
    }

    fs::copy_file(source, dst, fs::copy_options::overwrite_existing, ec);
    if (ec) {
        spdlog::error("config_library import failed: {}", ec.message());
        return {};
    }
    spdlog::info("config_library imported {} -> {}", source.string(), dst.string());
    return dst;
}

std::vector<LibraryEntry> list_configs(const fs::path& library_root, ConfigKind kind) {
    std::vector<LibraryEntry> out;
    auto dir = kind_dir(library_root, kind);
    std::error_code ec;
    if (!fs::is_directory(dir, ec)) {
        return out;
    }

    for (const auto& entry : fs::directory_iterator(dir, ec)) {
        if (!entry.is_regular_file()) {
            continue;
        }
        auto fname = entry.path().filename().string();
        out.push_back({entry.path(), std::move(fname)});
    }
    std::sort(out.begin(), out.end(),
              [](const auto& a, const auto& b) { return a.filename < b.filename; });
    return out;
}

bool remove_config(const fs::path& entry_path) {
    std::error_code ec;
    bool removed = fs::remove(entry_path, ec);
    if (removed) {
        spdlog::info("config_library removed {}", entry_path.string());
    }
    return removed;
}

}  // namespace stc::core::config_library
