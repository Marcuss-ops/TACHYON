#include <gtest/gtest.h>
#include "tachyon/core/json/json_document.h"
#include <fstream>
#include <filesystem>

namespace tachyon::core::json::tests {

class JsonParserTest : public ::testing::Test {
protected:
    void SetUp() override {
        temp_file = std::filesystem::temp_directory_path() / "test_simdjson.json";
    }

    void TearDown() override {
        if (std::filesystem::exists(temp_file)) {
            std::filesystem::remove(temp_file);
        }
    }

    std::filesystem::path temp_file;

    void write_temp_json(const std::string& content) {
        std::ofstream f(temp_file, std::ios::out | std::ios::trunc);
        f << content;
    }
};

TEST_F(JsonParserTest, ParseValidString) {
    JsonDocument doc;
    std::string error;
    bool ok = doc.parse_string(R"({"key": "value", "num": 123, "flag": true})", &error);
    
#if defined(TACHYON_ENABLE_SIMDJSON)
    EXPECT_TRUE(ok) << "Parse failed: " << error;
#else
    EXPECT_FALSE(ok);
#endif
}

TEST_F(JsonParserTest, ParseInvalidString) {
    JsonDocument doc;
    std::string error;
    bool ok = doc.parse_string(R"({"key": "value",)", &error);
    
    EXPECT_FALSE(ok);
#if defined(TACHYON_ENABLE_SIMDJSON)
    EXPECT_FALSE(error.empty());
#endif
}

TEST_F(JsonParserTest, ParseValidFile) {
    write_temp_json(R"({"name": "tachyon", "version": 0.1})");
    
    JsonDocument doc;
    std::string error;
    bool ok = doc.parse_file(temp_file.string(), &error);
    
#if defined(TACHYON_ENABLE_SIMDJSON)
    EXPECT_TRUE(ok) << "Parse failed: " << error;
#else
    EXPECT_FALSE(ok);
#endif
}

TEST_F(JsonParserTest, ParseNonExistentFile) {
    JsonDocument doc;
    std::string error;
    bool ok = doc.parse_file("non_existent_file_xyz.json", &error);
    
    EXPECT_FALSE(ok);
    EXPECT_FALSE(error.empty());
}

} // namespace tachyon::core::json::tests
