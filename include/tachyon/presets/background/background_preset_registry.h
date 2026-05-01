#pragma once
#include "tachyon/core/spec/schema/objects/layer_spec.h"
#include <optional>
#include <string>
#include <vector>

namespace tachyon::presets {

/**
 * Builds a procedural background layer based on a preset ID.
 */
std::optional<LayerSpec> build_background_preset(std::string_view id, int width, int height);

/**
 * Lists all available background preset IDs.
 */
std::vector<std::string> list_background_presets();

} // namespace tachyon::presets
