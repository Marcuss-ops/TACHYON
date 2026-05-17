#pragma once
#include <vector>
#include <optional>
#include <cstdint>
#include <span>

namespace tachyon::compression {

// Compress data with zstd. Returns empty vector on failure.
std::vector<std::uint8_t> compress(std::span<const std::uint8_t> data, int level = 3);

// Decompress zstd-compressed data. Returns nullopt on error.
std::optional<std::vector<std::uint8_t>> decompress(std::span<const std::uint8_t> compressed);

// Decompress only if data starts with the zstd magic; returns nullopt otherwise.
// Allows transparent backward-compatibility with uncompressed files.
std::optional<std::vector<std::uint8_t>> try_decompress(std::span<const std::uint8_t> data);

} // namespace tachyon::compression
