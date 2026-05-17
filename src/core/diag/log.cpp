#include "tachyon/core/diag/log.h"
#include <spdlog/sinks/stdout_color_sinks.h>
#include <cstdlib>
#include <string_view>

namespace tachyon::diag {

void init_logging() {
    auto sink = std::make_shared<spdlog::sinks::stderr_color_sink_mt>();
    sink->set_pattern("[%^%l%$] %v");

    auto logger = std::make_shared<spdlog::logger>("tachyon", std::move(sink));
    logger->set_level(spdlog::level::info);
    spdlog::set_default_logger(std::move(logger));

    if (const char* env = std::getenv("TACHYON_LOG_LEVEL")) {
        std::string_view level(env);
        if      (level == "trace") spdlog::set_level(spdlog::level::trace);
        else if (level == "debug") spdlog::set_level(spdlog::level::debug);
        else if (level == "warn")  spdlog::set_level(spdlog::level::warn);
        else if (level == "error") spdlog::set_level(spdlog::level::err);
    }
}

} // namespace tachyon::diag
