#pragma once

#include "tachyon/presets/effects/effect_specs.h"
#include "tachyon/core/registry/typed_registry.h"
#include <string>
#include <string_view>
#include <vector>

namespace tachyon::presets {

/**
 * @brief Registry for effect kinds (low-level rendering types).
 * Consolidates all rendering effects available in the engine.
 */
class EffectKindRegistry : public registry::TypedRegistry<EffectKindSpec> {
public:
    EffectKindRegistry() = default;
    
    ~EffectKindRegistry() = default;
};

} // namespace tachyon::presets
