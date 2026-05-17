#include "tachyon/core/diag/log.h"
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/stdout_sinks.h>
#include <cstdlib>
#include <string_view>
#include <algorithm>

namespace tachyon::diag {

namespace {
spdlog::level::level_enum parse_level(std::string_view level) {
    if (level == "trace") return spdlog::level::trace;
    if (level == "debug") return spdlog::level::debug;
    if (level == "warn")  return spdlog::level::warn;
    if (level == "error") return spdlog::level::err;
    if (level == "critical") return spdlog::level::critical;
    return spdlog::level::info;
}
}

void init_logging() {
    LogConfig config;
    
    if (const char* env_format = std::getenv("TACHYON_LOG_FORMAT")) {
        std::string_view format(env_format);
        if (format == "json") {
            config.format = LogFormat::Json;
        }
    }

    if (const char* env_level = std::getenv("TACHYON_LOG_LEVEL")) {
        config.level = env_level;
    }

    init_logging(config);
}

void init_logging(const LogConfig& config) {
    std::shared_ptr<spdlog::sinks::sink> sink;

    if (config.format == LogFormat::Json) {
        // Plain stderr sink for JSON (no colors)
        sink = std::make_shared<spdlog::sinks::stderr_sink_mt>();
        // Pattern producing a valid JSON object per line
        // %E: Epoch in seconds, %f: microseconds
        // Or %Y-%m-%dT%H:%M:%S.%f (ISO 8601-ish)
        sink->set_pattern("{\"ts\":\"%Y-%m-%dT%H:%M:%S.%fZ\",\"level\":\"%l\",\"component\":\"%n\",\"message\":\"%v\"}");
    } else {
        sink = std::make_shared<spdlog::sinks::stderr_color_sink_mt>();
        sink->set_pattern("[%^%l%$] %v");
    }

    auto logger = std::make_shared<spdlog::logger>("tachyon", std::move(sink));
    logger->set_level(parse_level(config.level));
    
    // Set as global default
    spdlog::set_default_logger(std::move(logger));
}

} // namespace tachyon::diag
