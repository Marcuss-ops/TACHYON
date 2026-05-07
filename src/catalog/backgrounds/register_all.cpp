#include "tachyon/backgrounds.hpp"

namespace tachyon {

void register_builtin_background_descriptors() {
    BackgroundRegistry::instance().register_descriptor(make_solid_background_descriptor());
    BackgroundRegistry::instance().register_descriptor(make_gradient_background_descriptor());
    BackgroundRegistry::instance().register_descriptor(make_radial_gradient_background_descriptor());
    BackgroundRegistry::instance().register_descriptor(make_image_background_descriptor());
    BackgroundRegistry::instance().register_descriptor(make_video_background_descriptor());
}

} // namespace tachyon
