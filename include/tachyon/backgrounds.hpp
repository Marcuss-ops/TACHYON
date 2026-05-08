#pragma once

#include "tachyon/core/spec/schema/common/common_spec.h"
#include "tachyon/background_descriptor.h"
#include "tachyon/background_registry.h"

namespace tachyon {

/**
 * @brief Creates a descriptor for the solid color background.
 */
TACHYON_API BackgroundDescriptor make_solid_background_descriptor();

/**
 * @brief Creates a descriptor for the linear gradient background.
 */
TACHYON_API BackgroundDescriptor make_gradient_background_descriptor();

/**
 * @brief Creates a descriptor for the radial gradient background.
 */
TACHYON_API BackgroundDescriptor make_radial_gradient_background_descriptor();

/**
 * @brief Creates a descriptor for the image background.
 */
TACHYON_API BackgroundDescriptor make_image_background_descriptor();

/**
 * @brief Creates a descriptor for the video background.
 */
TACHYON_API BackgroundDescriptor make_video_background_descriptor();

} // namespace tachyon
