#pragma once

#include <string>
#include <filesystem>

namespace tachyon::io {

/**
 * @brief Resolves a path relative to a base directory.
 */
std::string resolve_relative_path(const std::string& base, const std::string& rel);

/**
 * @brief Resolves a path relative to the asset root.
 */
std::string resolve_relative_to_root(const std::string& rel);

/**
 * @brief Returns the extension of a path in lowercase.
 */
std::string extension_lower(const std::string& path);

} // namespace tachyon::io
