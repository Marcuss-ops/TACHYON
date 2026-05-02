#include "tachyon/core/json/json_reader.h"
#include "tachyon/core/spec/parsing/spec_value_parsers.h"
#include <nlohmann/json.hpp>
#include <iostream>
#include <string>
#include <cmath>

using json = nlohmann::json;
using namespace tachyon;

namespace {

int g_failures = 0;

void check_true(bool condition, const std::string& message) {
    if (!condition) {
        ++g_failures;
        std::cerr << "FAIL: " << message << '\n';
    }
}

void check_false(bool condition, const std::string& message) {
    check_true(!condition, message);
}

} // namespace

bool run_json_reader_tests() {
    g_failures = 0;

    // ReadString
    {
        json j = {{"key", "value"}};
        std::string out;
        check_true(read_string(j, "key", out), "ReadString should succeed");
        check_true(out == "value", "ReadString value match");
        
        check_false(read_string(j, "missing", out), "ReadString should fail for missing key");
    }

    // ReadBool
    {
        json j = {{"flag", true}};
        bool out = false;
        check_true(read_bool(j, "flag", out), "ReadBool should succeed");
        check_true(out, "ReadBool value match");
    }

    // ReadDouble
    {
        json j = {{"val", 3.14}};
        double out = 0.0;
        check_true(read_double(j, "val", out), "ReadDouble should succeed");
        check_true(std::abs(out - 3.14) < 1e-6, "ReadDouble value match");
    }

    // ParseVector2
    {
        json j = {10.0, 20.0};
        math::Vector2 out;
        check_true(parse_vector2_value(j, out), "ParseVector2 (array) should succeed");
        check_true(out.x == 10.0f && out.y == 20.0f, "Vector2 values match");
        
        json obj = {{"x", 3.0}, {"y", 4.0}};
        check_true(parse_vector2_value(obj, out), "ParseVector2 (object) should succeed");
        check_true(out.x == 3.0f && out.y == 4.0f, "Vector2 values match (object)");
    }

    // ParseColor
    {
        json j = {255, 128, 64, 32};
        ColorSpec out;
        check_true(parse_color_value(j, out), "ParseColor (array) should succeed");
        check_true(out.r == 255 && out.g == 128 && out.b == 64 && out.a == 32, "Color values match");
        
        json hex = "#ff0000";
        check_true(parse_color_value(hex, out), "ParseColor (hex) should succeed");
        check_true(out.r == 255 && out.g == 0 && out.b == 0, "Color hex match");
    }

    return g_failures == 0;
}
