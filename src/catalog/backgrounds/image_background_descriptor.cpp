#include "tachyon/backgrounds.hpp"
#include "tachyon/core/spec/schema/objects/layer_spec.h"
#include <functional>

namespace tachyon {

BackgroundDescriptor make_image_background_descriptor() {
    BackgroundDescriptor desc;
    desc.id = "tachyon.background.image";
    desc.aliases = {"image", "img"};
    desc.kind = BackgroundKind::Image;
    desc.params = registry::ParameterSchema{
        {"path", "Image Path", "Path to image file", std::string("")}
    };
    desc.capabilities.supports_cpu = true;
    desc.capabilities.supports_gpu = true;
    desc.display_name = "Image";
    desc.description = "Image background";

    // Build function
    desc.build = [](const registry::ParameterBag& params) -> LayerSpec {
        LayerSpec spec;
        // TODO: Implement image background layer creation
        return spec;
    };

    return desc;
}

} // namespace tachyon
