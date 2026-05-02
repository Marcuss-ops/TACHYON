#include <gtest/gtest.h>
#include "tachyon/core/json/json_reader.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

TEST(JsonReaderTest, ReadString) {
    json j = {{"key", "value"}};
    std::string out;
    EXPECT_TRUE(read_string(j, "key", out));
    EXPECT_EQ(out, "value");
    
    EXPECT_FALSE(read_string(j, "missing", out));
    
    j["key"] = nullptr;
    EXPECT_FALSE(read_string(j, "key", out));
}

TEST(JsonReaderTest, ReadBool) {
    json j = {{"flag", true}};
    bool out = false;
    EXPECT_TRUE(read_bool(j, "flag", out));
    EXPECT_TRUE(out);
    
    j["flag"] = "not_bool";
    EXPECT_FALSE(read_bool(j, "flag", out));
}

TEST(JsonReaderTest, ReadDouble) {
    json j = {{"val", 3.14}};
    double out = 0.0;
    EXPECT_TRUE(read_double(j, "val", out));
    EXPECT_NEAR(out, 3.14, 1e-6);
    
    j["val"] = "string";
    EXPECT_FALSE(read_double(j, "val", out));
}

TEST(JsonReaderTest, ReadInt64) {
    json j = {{"num", 42}};
    std::int64_t out = 0;
    EXPECT_TRUE(read_int64(j, "num", out));
    EXPECT_EQ(out, 42);
    
    j["num"] = 3.14;
    EXPECT_FALSE(read_int64(j, "num", out));
}

TEST(JsonReaderTest, ReadVector2) {
    json j = {1.0, 2.0};
    math::Vector2 out;
    EXPECT_TRUE(read_vector2(j, out));
    EXPECT_FLOAT_EQ(out.x, 1.0f);
    EXPECT_FLOAT_EQ(out.y, 2.0f);
    
    json obj = {{"x", 3.0}, {"y", 4.0}};
    EXPECT_TRUE(read_vector2(obj, out));
    EXPECT_FLOAT_EQ(out.x, 3.0f);
    EXPECT_FLOAT_EQ(out.y, 4.0f);
}

TEST(JsonReaderTest, ReadColor) {
    json j = {255, 128, 64, 32};
    ColorSpec out;
    EXPECT_TRUE(read_color(j, out));
    EXPECT_EQ(out.r, 255);
    EXPECT_EQ(out.g, 128);
    EXPECT_EQ(out.b, 64);
    EXPECT_EQ(out.a, 32);
    
    json obj = {{"r", 10}, {"g", 20}, {"b", 30}};
    EXPECT_TRUE(read_color(obj, out));
    EXPECT_EQ(out.r, 10);
    EXPECT_EQ(out.g, 20);
    EXPECT_EQ(out.b, 30);
    EXPECT_EQ(out.a, 255);
}
