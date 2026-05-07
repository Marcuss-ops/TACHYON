#pragma once

#include "tachyon/background_descriptor.h"

namespace tachyon {

/**
 * @brief Creates a descriptor for the solid color background.
 */
BackgroundDescriptor make_solid_background_descriptor();

/**
 * @brief Creates a descriptor for the linear gradient background.
 */
BackgroundDescriptor make_gradient_background_descriptor();

/**
 * @brief Creates a descriptor for the radial gradient background.
 */
BackgroundDescriptor make_radial_gradient_background_descriptor();

/**
 * @brief Creates a descriptor for the image background.
 */
BackgroundDescriptor make_image_background_descriptor();

/**
 * @brief Creates a descriptor for the video background.
 */
BackgroundDescriptor make_video_background_descriptor();

/**
 * @brief Registers all built-in background descriptors.
 */
void register_builtin_background_descriptors();

} // namespace tachyon
