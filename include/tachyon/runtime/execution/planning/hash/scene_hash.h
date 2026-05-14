#pragma once

#include "tachyon/core/spec/schema/objects/scene_spec.h"
#include <cstdint>

namespace tachyon {

/**
 * @brief Computes a stable hash of the entire scene content.
 * 
 * Used for cache invalidation and determinism checks.
 */
[[nodiscard]] std::uint64_t hash_scene_content(const SceneSpec& scene);

} // namespace tachyon
