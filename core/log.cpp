#include "core/log.hpp"

#include <spdlog/sinks/msvc_sink.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/spdlog.h>

#include <atomic>
#include <memory>
#include <vector>

namespace stc::core::log {
namespace {

std::atomic<bool> initialized{false};

}  // namespace

void init(const std::filesystem::path& dir, const char* name) {
    bool expected = false;
    if (!initialized.compare_exchange_strong(expected, true)) {
        return;
    }

    std::error_code ec;
    std::filesystem::create_directories(dir / "logs", ec);

    auto file = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
        (dir / "logs" / (std::string(name) + ".log")).string(),
        1024 * 1024 * 4,  // 4 MB per file
        5);
    auto debug = std::make_shared<spdlog::sinks::msvc_sink_mt>();

    std::vector<spdlog::sink_ptr> sinks{file, debug};
    auto logger = std::make_shared<spdlog::logger>(name, sinks.begin(), sinks.end());
    logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%t] %v");
    logger->set_level(spdlog::level::debug);
    logger->flush_on(spdlog::level::warn);

    spdlog::set_default_logger(logger);
    spdlog::info("Logger up. Writing to {}", (dir / "logs").string());
}

void flush_all() {
    if (auto l = spdlog::default_logger()) {
        l->flush();
    }
}

}  // namespace stc::core::log
