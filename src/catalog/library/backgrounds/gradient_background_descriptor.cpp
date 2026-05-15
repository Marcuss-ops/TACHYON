#include "tachyon/backgrounds.hpp"
#include "tachyon/core/spec/schema/objects/layer_spec.h"
#include <functional>

namespace tachyon {

BackgroundDescriptor make_gradient_background_descriptor() {
    BackgroundDescriptor desc;
    desc.id = "tachyon.background.gradient";
    desc.aliases = {"linear_gradient", "linear-gradient"};
    desc.kind = BackgroundKind::LinearGradient;
    desc.params = registry::ParameterSchema{
        {"color_start", "Start Color", "Gradient start color", ColorSpec(0, 0, 0)},
        {"color_end", "End Color", "Gradient end color", ColorSpec(255, 255, 255)},
        {"angle", "Angle", "Gradient angle in degrees", 0.0}
    };
    desc.capabilities.supports_cpu = true;
    desc.capabilities.supports_gpu = true;
    desc.display_name = "Linear Gradient";
    desc.description = "Linear gradient background";

    // Build function
    desc.build = [](const registry::ParameterBag& params) -> LayerSpec {
        LayerSpec spec;
        spec.identity.type = LayerType::Procedural;
        spec.identity.name = "Linear Gradient Background";
        
        ProceduralSpec proc;
        proc.kind = "tachyon.background.kind.linear_gradient";
        proc.color_a.keyframes = {{0.0, params.get_or<ColorSpec>("color_start", ColorSpec(0, 0, 0))}};
        proc.color_b.keyframes = {{0.0, params.get_or<ColorSpec>("color_end", ColorSpec(255, 255, 255))}};
        proc.angle.keyframes = {{0.0, params.get_or<double>("angle", 0.0)}};
        spec.source = ProceduralSource{proc.kind, proc};
        
        return spec;
    };

    return desc;
}

} // namespace tachyon
