#pragma once

#include "tachyon/runtime/core/compiled_scene.h"
#include <vector>
#include <cstdint>
#include <optional>
#include <filesystem>

namespace tachyon::runtime {

/**
 * @brief Handles serialization and deserialization of CompiledScene to/from TBF (Tachyon Binary Format).
 */
class TBFCodec {
public:
    /**
     * @brief Encodes a CompiledScene into a binary buffer.
     */
    static std::vector<std::uint8_t> encode(const CompiledScene& scene);

    /**
     * @brief Decodes a CompiledScene from a binary buffer.
     * Returns std::nullopt if the buffer is invalid or version mismatch.
     */
    static std::optional<CompiledScene> decode(const std::vector<std::uint8_t>& buffer);

    /**
     * @brief Helper to save a CompiledScene to a file.
     */
    static bool save_to_file(const CompiledScene& scene, const std::filesystem::path& path);

    /**
     * @brief Helper to load a CompiledScene from a file.
     */
    static std::optional<CompiledScene> load_from_file(const std::filesystem::path& path);
};

} // namespace tachyon::runtime
