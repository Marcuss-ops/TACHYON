#pragma once

#include "tachyon/core/spec/schema/objects/background_spec.h"
#include "tachyon/core/spec/schema/objects/layer_spec.h"
#include <vector>
#include <string>

namespace tachyon::presets {

/**
 * @brief Detailed result of a background resolution attempt.
 */
struct BackgroundResolutionResult {
    enum class Status {
        Ok,
        EmptyByDesign,         ///< No background intended
        InvalidColor,          ///< Color parsing failed
        UnknownPreset,         ///< Preset ID not found in registry
        UnsupportedComponent,  ///< Component-type background not yet implemented
        AssetMissing,          ///< Referenced image/video not found
        CatalogMismatch        ///< Registry and Catalog are out of sync
    };

    Status status{Status::Ok};
    std::vector<LayerSpec> layers;
    std::string error_message;
};

/// Converts a BackgroundSpec into one or more normal LayerSpec objects.
/// Background is an authoring concept only — it must not render pixels directly.
[[nodiscard]] BackgroundResolutionResult resolve_background(
    const BackgroundSpec& bg,
    int width,
    int height,
    double duration = 5.0
);

// Legacy/Compatibility wrapper
[[nodiscard]] std::vector<LayerSpec> resolve_background_to_layers(
    const BackgroundSpec& bg,
    int width,
    int height,
    double duration = 5.0
);

[[nodiscard]] std::vector<LayerSpec> resolve_background_to_layers_checked(
    const BackgroundSpec& bg,
    int width,
    int height,
    double duration = 5.0
);

} // namespace tachyon::presets
