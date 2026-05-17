#pragma once
#include <spdlog/spdlog.h>
#include <string>

namespace tachyon::diag {

enum class LogFormat {
    Text,
    Json
};

struct LogConfig {
    LogFormat format{LogFormat::Text};
    std::string level{"info"};
    bool stderr_output{true};
};

/**
 * @brief Initializes the logging system with default settings or environment variables.
 * TACHYON_LOG_FORMAT: text (default) or json
 * TACHYON_LOG_LEVEL: trace, debug, info (default), warn, error, critical
 */
void init_logging();

/**
 * @brief Initializes the logging system with a specific configuration.
 */
void init_logging(const LogConfig& config);

template<typename... Args>
void info(spdlog::format_string_t<Args...> fmt, Args&&... args) {
    spdlog::info(fmt, std::forward<Args>(args)...);
}

template<typename... Args>
void warn(spdlog::format_string_t<Args...> fmt, Args&&... args) {
    spdlog::warn(fmt, std::forward<Args>(args)...);
}

template<typename... Args>
void error(spdlog::format_string_t<Args...> fmt, Args&&... args) {
    spdlog::error(fmt, std::forward<Args>(args)...);
}

template<typename... Args>
void critical(spdlog::format_string_t<Args...> fmt, Args&&... args) {
    spdlog::critical(fmt, std::forward<Args>(args)...);
}

template<typename... Args>
void debug(spdlog::format_string_t<Args...> fmt, Args&&... args) {
    spdlog::debug(fmt, std::forward<Args>(args)...);
}

template<typename... Args>
void trace(spdlog::format_string_t<Args...> fmt, Args&&... args) {
    spdlog::trace(fmt, std::forward<Args>(args)...);
}

} // namespace tachyon::diag
