#pragma once
#include "tachyon/core/spec/schema/objects/scene_spec.h"

namespace tachyon::core {

/**
 * @brief Normalizes a SceneSpec by resolving authoring shorthands into canonical runtime fields.
 * 
 * This includes:
 * - Resolving LayerTiming from start_time, in_point, out_point, duration.
 * - (Future) Resolving preset aliases.
 * - (Future) Stripping legacy fields like type_string.
 */
class SceneNormalizer {
public:
    /**
     * @brief Normalizes the scene in-place.
     */
    static void normalize(SceneSpec& scene);
};

} // namespace tachyon::core
