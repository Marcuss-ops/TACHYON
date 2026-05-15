#include "tachyon/backgrounds.hpp"
#include "tachyon/core/spec/schema/objects/layer_spec.h"
#include <functional>

namespace tachyon {

BackgroundDescriptor make_video_background_descriptor() {
    BackgroundDescriptor desc;
    desc.id = "tachyon.background.video";
    desc.aliases = {"video", "vid"};
    desc.kind = BackgroundKind::Video;
    desc.params = registry::ParameterSchema{
        {"path", "Video Path", "Path to video file", std::string("")},
        {"loop", "Loop", "Whether to loop the video", true},
        {"mute", "Mute", "Whether to mute audio", true}
    };
    desc.capabilities.supports_cpu = true;
    desc.capabilities.supports_gpu = true;
    desc.display_name = "Video";
    desc.description = "Video background";

    // Build function
    desc.build = [](const registry::ParameterBag& params) -> LayerSpec {
        LayerSpec spec;
        spec.identity.type = LayerType::Video;
        spec.identity.name = "Video Background";
        
        spec.source = MediaSource{ { params.get_or<std::string>("path", "") } };
        spec.playback.loop = params.get_or<bool>("loop", true);
        
        return spec;
    };

    return desc;
}

} // namespace tachyon
