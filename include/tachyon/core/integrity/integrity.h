#pragma once
#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>

namespace tachyon::core::integrity {

/**
 * @brief Computes the CRC32C checksum of a memory buffer.
 * 
 * Uses hardware acceleration (SSE4.2 / ARMv8 CRC) if available.
 */
uint32_t compute_crc32c(const uint8_t* data, size_t size);

/**
 * @brief Computes the CRC32C checksum of a file.
 */
uint32_t compute_file_crc32c(const std::string& path);

} // namespace tachyon::core::integrity
