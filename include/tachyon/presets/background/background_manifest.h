#pragma once

#include "tachyon/presets/background/background_specs.h"
#include "tachyon/background_descriptor.h"
#include <vector>

namespace tachyon::presets {

/**
 * @brief Manifest that consolidates all background generation.
 * 
 * This is the single canonical source for both runtime background descriptors
 * and authoring background preset specs.
 */
class BackgroundManifest {
public:
    /**
     * @brief Generate all runtime background descriptors from all categories.
     * @return Vector of all background descriptors.
     */
    std::vector<BackgroundDescriptor> generate_descriptors() const;

    /**
     * @brief Generate all authoring background preset specs.
     * @return Vector of all background preset specs.
     */
    std::vector<BackgroundPresetSpec> generate_preset_specs() const;

    /**
     * @brief Generate all background kind specs (low-level procedural types).
     * @return Vector of all background kind specs.
     */
    std::vector<BackgroundKindSpec> generate_kind_specs() const;
};

} // namespace tachyon::presets
