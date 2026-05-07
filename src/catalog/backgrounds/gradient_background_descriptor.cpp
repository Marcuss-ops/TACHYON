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
        // TODO: Implement gradient background layer creation
        return spec;
    };

    return desc;
}

} // namespace tachyon
