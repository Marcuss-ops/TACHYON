#include "tachyon/backgrounds.hpp"
#include "tachyon/background_registry.h"

namespace tachyon {

void register_builtin_background_descriptors(BackgroundRegistry& registry) {
    registry.register_descriptor(make_solid_background_descriptor());
    registry.register_descriptor(make_gradient_background_descriptor());
    registry.register_descriptor(make_radial_gradient_background_descriptor());
    registry.register_descriptor(make_image_background_descriptor());
    registry.register_descriptor(make_video_background_descriptor());
}

} // namespace tachyon
