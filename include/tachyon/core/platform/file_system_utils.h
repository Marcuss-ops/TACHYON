#pragma once

#include <string>
#include <system_error>

namespace tachyon::core::platform {

/**
 * @brief Removes a file without throwing exceptions.
 * @param path The path to the file to remove.
 * @return True if the file was removed or didn't exist, false otherwise.
 */
bool remove_file_silent(const std::string& path);

} // namespace tachyon::core::platform
