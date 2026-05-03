#include "tachyon/core/json/json_reader.h"
#include <nlohmann/json.hpp>
#include <iostream>

using json = nlohmann::json;
using namespace tachyon;

bool run_json_reader_tests() {
    int failures = 0;
    
    // Test read_string
    {
        json j = {{"key", "value"}};
        std::string out;
        if (!read_string(j, "key", out) || out != "value") {
            std::cerr << "FAIL: read_string basic\n";
            ++failures;
        }
        
        if (read_string(j, "missing", out)) {
            std::cerr << "FAIL: read_string missing key should return false\n";
            ++failures;
        }
        
        j["key"] = nullptr;
        if (read_string(j, "key", out)) {
            std::cerr << "FAIL: read_string null value should return false\n";
            ++failures;
        }
    }
    
    // Test read_bool
    {
        json j = {{"flag", true}};
        bool out = false;
        if (!read_bool(j, "flag", out) || !out) {
            std::cerr << "FAIL: read_bool basic\n";
            ++failures;
        }
        
        j["flag"] = "not_bool";
        if (read_bool(j, "flag", out)) {
            std::cerr << "FAIL: read_bool non-bool should return false\n";
            ++failures;
        }
    }
    
    // Test read_double
    {
        json j = {{"val", 3.14}};
        double out = 0.0;
        if (!read_double(j, "val", out) || std::abs(out - 3.14) > 1e-6) {
            std::cerr << "FAIL: read_double basic\n";
            ++failures;
        }
        
        j["val"] = "string";
        if (read_double(j, "val", out)) {
            std::cerr << "FAIL: read_double non-number should return false\n";
            ++failures;
        }
    }
    
    // Test read_int64
    {
        json j = {{"num", 42}};
        std::int64_t out = 0;
        if (!read_int64(j, "num", out) || out != 42) {
            std::cerr << "FAIL: read_int64 basic\n";
            ++failures;
        }
        
        j["num"] = 3.14;
        if (read_int64(j, "num", out)) {
            std::cerr << "FAIL: read_int64 non-integer should return false\n";
            ++failures;
        }
    }
    
    if (failures == 0) {
        std::cerr << "[OK] json_reader_tests passed\n";
        return true;
    } else {
        std::cerr << "[FAIL] json_reader_tests failed with " << failures << " errors\n";
        return false;
    }
}
