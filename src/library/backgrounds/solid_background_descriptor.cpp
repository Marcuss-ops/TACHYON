#include "tachyon/backgrounds.hpp"
#include "tachyon/core/spec/schema/objects/layer_spec.h"
#include <functional>

namespace tachyon {

BackgroundDescriptor make_solid_background_descriptor() {
    BackgroundDescriptor desc;
    desc.id = "tachyon.background.solid";
    desc.aliases = {"solid", "color"};
    desc.kind = BackgroundKind::Solid;
    desc.params = registry::ParameterSchema{
        {"color", "Color", "Background color", ColorSpec(0, 0, 0)}
    };
    desc.capabilities.supports_cpu = true;
    desc.capabilities.supports_gpu = true;
    desc.display_name = "Solid Color";
    desc.description = "Solid color background";

    // Build function
    desc.build = [](const registry::ParameterBag& params) -> LayerSpec {
        LayerSpec spec;
        spec.type = LayerType::Solid;
        spec.name = "Solid Background";
        
        auto color = params.get_or<ColorSpec>("color", ColorSpec(0, 0, 0));
        spec.fill_color.keyframes = {{0.0, color}};
        
        return spec;
    };

    return desc;
}

} // namespace tachyon
