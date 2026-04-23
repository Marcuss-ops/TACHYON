#pragma once

#include "tachyon/runtime/core/data/compiled_scene.h"
#include <vector>
#include <cstdint>
#include <optional>
#include <filesystem>

namespace tachyon::runtime {

/**
 * @brief Handles serialization and deserialization of CompiledScene to/from TBF (Tachyon Binary Format).
 * 
 * Versioning:
 * - Version 1: Initial format
 * - Version 2: Added mask_feather to CompiledLayer
 * - Version 3: Added audio tracks to CompiledComposition
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
     * Handles migration for older versions.
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

    /**
     * @brief Returns the current binary format version.
     */
    static constexpr std::uint16_t current_version() { return 3; }

    /**
     * @brief Migrates an older format to the current version.
     */
    static CompiledScene migrate(const CompiledScene& scene, std::uint16_t from_version);
};

} // namespace tachyon::runtime
