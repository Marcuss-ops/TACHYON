#pragma once
#include <filesystem>
#include <string>

namespace tachyon::test {

/**
 * @brief Returns absolute path to a test fixture file.
 * @param relative_path Path relative to tests/ directory (e.g., "fixtures/scenes/my_scene.json")
 * @return Absolute path to the fixture file
 */
inline std::filesystem::path fixture_path(const std::string& relative_path) {
#ifdef TACHYON_TESTS_SOURCE_DIR
    return std::filesystem::path(TACHYON_TESTS_SOURCE_DIR) / relative_path;
#else
    return std::filesystem::current_path() / relative_path;
#endif
}

} // namespace tachyon::test
