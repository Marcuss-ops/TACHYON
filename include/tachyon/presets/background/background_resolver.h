#pragma once

#include "tachyon/core/spec/schema/objects/background_spec.h"
#include "tachyon/core/spec/schema/objects/layer_spec.h"
#include <vector>

namespace tachyon::presets {

/// Converts a BackgroundSpec into one or more normal LayerSpec objects.
/// Background is an authoring concept only — it must not render pixels directly.
[[nodiscard]] std::vector<LayerSpec> resolve_background_to_layers(
    const BackgroundSpec& bg,
    int width,
    int height,
    double duration = 5.0
);

} // namespace tachyon::presets
