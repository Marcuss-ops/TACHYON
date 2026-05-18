#include <gtest/gtest.h>
#include "tachyon/core/diag/log.h"
#include <sstream>
#include <iostream>

namespace tachyon::diagnostics {

bool run_logging_test() {
    using namespace tachyon::diag;
    // 1. Test standard logging formatting
    {
        std::ostringstream oss;
        LogConfig config;
        config.format = LogFormat::Json;
        config.level = "info";
        config.test_stream = &oss;

        init_logging(config);

        // Retrieve spdlog default logger and log through the facade
        spdlog::info("Test message with value {}", 42);

        std::string output = oss.str();
        if (output.empty()) {
            std::cerr << "[LoggingTest] FAIL: JSON output is empty\n";
            return false;
        }
        if (output.front() != '{') {
            std::cerr << "[LoggingTest] FAIL: JSON output does not start with {\n";
            return false;
        }
        if (output.find("\"level\":\"info\"") == std::string::npos) {
            std::cerr << "[LoggingTest] FAIL: Level not formatted as info\n";
            return false;
        }
        if (output.find("\"component\":\"tachyon\"") == std::string::npos) {
            std::cerr << "[LoggingTest] FAIL: Component not formatted as tachyon\n";
            return false;
        }
        if (output.find("\"message\":\"Test message with value 42\"") == std::string::npos) {
            std::cerr << "[LoggingTest] FAIL: Message content mismatch\n";
            return false;
        }
    }

    // 2. Test escape safety (quotes, backslashes, newlines)
    {
        std::ostringstream oss;
        LogConfig config;
        config.format = LogFormat::Json;
        config.level = "info";
        config.test_stream = &oss;

        init_logging(config);

        // Log a message containing JSON special characters
        spdlog::info("Message with \"quotes\", \\backslashes\\ and\nnewlines");

        std::string output = oss.str();
        
        // The expected escaped message payload is:
        // "Message with \"quotes\", \\\\backslashes\\\\ and\\nnewlines"
        if (output.find("Message with \\\"quotes\\\", \\\\backslashes\\\\ and\\nnewlines") == std::string::npos) {
            std::cerr << "[LoggingTest] FAIL: Special characters were not escaped correctly in output: " << output << "\n";
            return false;
        }
    }

    std::cout << "[LoggingTest] ALL logging tests passed successfully!\n";
    return true;
}

} // namespace tachyon::diagnostics
