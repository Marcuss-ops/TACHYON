#pragma once

#include "tachyon/core/spec/schema/common/common_spec.h"
#include "tachyon/renderer2d/core/framebuffer.h"

namespace tachyon::renderer2d {
    struct EffectParams;
}
#include <string>
#include <functional>
#include <vector>

namespace tachyon::renderer2d {

/**
 * @brief Requirements for auxiliary surfaces (e.g., masks, displacement maps).
 */
struct AuxSurfaceRequirement {
    std::string param_name;    ///< Internal name used by the effect implementation
    std::string source_key;    ///< Key in EffectSpec::strings to look up the layer ID
    std::string semantic;      ///< e.g., "mask", "displacement", "aux"
    bool is_required{true};
    std::string default_id;    ///< Optional default layer ID if source_key is missing
};

/**
 * @brief Descriptor for a rendering effect.
 */
struct EffectDescriptor {
    using Factory = std::function<void(
        const EffectSpec& spec,
        const SurfaceRGBA& input,
        SurfaceRGBA& output,
        const std::vector<const SurfaceRGBA*>& aux_surfaces,
        const EffectParams& params)>;

    std::string id;
    std::string category;
    Factory factory;
    
    std::vector<AuxSurfaceRequirement> aux_requirements;
    
    bool is_deterministic{true};
    bool supports_gpu_acceleration{false};
};

} // namespace tachyon::renderer2d
