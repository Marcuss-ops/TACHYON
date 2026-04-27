#include <gtest/gtest.h>
#include "tachyon/core/logging.h"
#include <spdlog/sinks/ostream_sink.h>
#include <sstream>

namespace {

std::string capture_log(spdlog::level::level_enum level, std::function<void()> fn) {
    std::ostringstream oss;
    auto sink = std::make_shared<spdlog::sinks::ostream_sink_mt>(oss);
    sink->set_level(spdlog::level::trace);
    sink->set_pattern("%l %v");

    auto test_logger = std::make_shared<spdlog::logger>("test_capture", sink);
    test_logger->set_level(spdlog::level::trace);

    // Swap the global tachyon logger temporarily
    auto real = tachyon::detail::swap_logger_for_test(test_logger);
    fn();
    tachyon::detail::swap_logger_for_test(real);
    spdlog::drop("test_capture");

    return oss.str();
}

} // namespace

TEST(Logging, InfoMessageAppearsInOutput) {
    std::ostringstream oss;
    auto sink = std::make_shared<spdlog::sinks::ostream_sink_mt>(oss);
    sink->set_level(spdlog::level::trace);
    sink->set_pattern("%v");

    auto logger = std::make_shared<spdlog::logger>("tachyon_test", sink);
    logger->set_level(spdlog::level::trace);

    logger->info("hello logging");
    EXPECT_NE(oss.str().find("hello logging"), std::string::npos);
}

TEST(Logging, LoggerSingletonIsNotNull) {
    EXPECT_NE(tachyon::get_logger(), nullptr);
}

TEST(Logging, MacroDoesNotCrash) {
    TLOG_INFO("test message {}", 42);
    TLOG_WARN("warn {}", "x");
    TLOG_ERROR("error {}", 0);
    // If we get here, no crash
    SUCCEED();
}
