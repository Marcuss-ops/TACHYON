#pragma once

#include "tachyon/core/spec/schema/objects/layer_spec.h"
#include "tachyon/presets/preset_base.h"
#include "tachyon/presets/transition/transition_preset_registry.h"

namespace tachyon::presets {

/**
 * Creates a LayerSpec with basic properties (id, name, timing, transform, etc.)
 * common to all layer types.
 */
LayerSpec make_base_layer(
    const std::string& id,
    const std::string& name,
    const std::string& type,
    const LayerParams& p
);

/**
 * Applies enter and exit transitions to an existing layer.
 */
void apply_layer_transitions(
    LayerSpec& layer,
    const std::string& enter_id,
    double enter_duration,
    const std::string& exit_id,
    double exit_duration,
    const TransitionPresetRegistry& registry
);

} // namespace tachyon::presets
