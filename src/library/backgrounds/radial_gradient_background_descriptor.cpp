#include "tachyon/backgrounds.hpp"
#include "tachyon/core/spec/schema/objects/layer_spec.h"
#include <functional>

namespace tachyon {

BackgroundDescriptor make_radial_gradient_background_descriptor() {
    BackgroundDescriptor desc;
    desc.id = "tachyon.background.radial_gradient";
    desc.aliases = {"radial_gradient", "radial-gradient"};
    desc.kind = BackgroundKind::RadialGradient;
    desc.params = registry::ParameterSchema{
        {"color_start", "Start Color", "Gradient start color", ColorSpec(0, 0, 0)},
        {"color_end", "End Color", "Gradient end color", ColorSpec(255, 255, 255)},
        {"center_x", "Center X", "Center X position (0-1)", 0.5},
        {"center_y", "Center Y", "Center Y position (0-1)", 0.5}
    };
    desc.capabilities.supports_cpu = true;
    desc.capabilities.supports_gpu = true;
    desc.display_name = "Radial Gradient";
    desc.description = "Radial gradient background";

    // Build function
    desc.build = [](const registry::ParameterBag& params) -> LayerSpec {
        LayerSpec spec;
        spec.type = LayerType::Procedural;
        spec.name = "Radial Gradient Background";
        
        ProceduralSpec proc;
        proc.kind = "tachyon.background.kind.radial_gradient";
        proc.color_a.keyframes = {{0.0, params.get_or<ColorSpec>("color_start", ColorSpec(0, 0, 0))}};
        proc.color_b.keyframes = {{0.0, params.get_or<ColorSpec>("color_end", ColorSpec(255, 255, 255))}};
        proc.focal_x = params.get_or<double>("center_x", 0.5);
        proc.focal_y = params.get_or<double>("center_y", 0.5);
        spec.procedural = proc;
        
        return spec;
    };

    return desc;
}

} // namespace tachyon
