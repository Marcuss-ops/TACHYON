#pragma once

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <memory>

namespace tachyon {

inline std::shared_ptr<spdlog::logger>& get_logger() {
    static std::shared_ptr<spdlog::logger> logger = [] {
        auto l = spdlog::stdout_color_mt("tachyon");
        l->set_level(spdlog::level::warn);
        l->set_pattern("[%H:%M:%S.%e] [%n] [%^%l%$] [t%t] %v");
        return l;
    }();
    return logger;
}

namespace detail {
inline std::shared_ptr<spdlog::logger> swap_logger_for_test(
        std::shared_ptr<spdlog::logger> replacement) {
    auto prev = get_logger();
    get_logger() = std::move(replacement);
    return prev;
}
} // namespace detail

} // namespace tachyon

// Livello controllabile via TACHYON_LOG_LEVEL a compile time o runtime
#define TLOG_TRACE(...)    ::tachyon::get_logger()->trace(__VA_ARGS__)
#define TLOG_DEBUG(...)    ::tachyon::get_logger()->debug(__VA_ARGS__)
#define TLOG_INFO(...)     ::tachyon::get_logger()->info(__VA_ARGS__)
#define TLOG_WARN(...)     ::tachyon::get_logger()->warn(__VA_ARGS__)
#define TLOG_ERROR(...)    ::tachyon::get_logger()->error(__VA_ARGS__)
#define TLOG_CRITICAL(...) ::tachyon::get_logger()->critical(__VA_ARGS__)
