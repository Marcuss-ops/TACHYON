#include <gtest/gtest.h>
#include "tachyon/core/diag/log.h"
#include <spdlog/sinks/ostream_sink.h>
#include <sstream>

namespace tachyon::diagnostics {

bool run_logging_test() {
    std::ostringstream oss;
    auto oss_sink = std::make_shared<spdlog::sinks::ostream_sink_mt>(oss);
    
    // We need to manually setup the logger to use our ostream sink for testing
    auto test_logger = std::make_shared<spdlog::logger>("test_json", oss_sink);
    test_logger->set_level(spdlog::level::info);
    
    // Pattern from log.cpp
    test_logger->set_pattern("{\"ts\":\"%Y-%m-%dT%H:%M:%S.%fZ\",\"level\":\"%l\",\"component\":\"%n\",\"message\":\"%v\"}");
    
    test_logger->info("Test message with value {}", 42);
    
    std::string output = oss.str();
    
    if (output.empty()) return false;
    if (output.front() != '{') return false;
    if (output.find("\"level\":\"info\"") == std::string::npos) return false;
    if (output.find("\"component\":\"test_json\"") == std::string::npos) return false;
    if (output.find("\"message\":\"Test message with value 42\"") == std::string::npos) return false;
    if (output.find("\"ts\":") == std::string::npos) return false;
    
    return true;
}

} // namespace tachyon::diagnostics
