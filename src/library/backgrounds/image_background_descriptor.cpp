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
        spec.identity.type = LayerType::Image;
        spec.identity.name = "Image Background";
        
        spec.transform.width = static_cast<int>(params.get_or<float>("width", 1920.0f));
        spec.transform.height = static_cast<int>(params.get_or<float>("height", 1080.0f));
        spec.playback.timing.duration = params.get_or<double>("duration", 10.0);
        
        spec.source = MediaSource{ { params.get_or<std::string>("path", params.get_or<std::string>("asset_path", "")) } };
        
        return spec;
    };

    return desc;
}

} // namespace tachyon
