#pragma once

#include "tachyon/renderer2d/core/framebuffer.h"
#include <string>
#include <unordered_map>

namespace tachyon::renderer2d {

/**
 * @brief Runtime parameters for an effect.
 */
struct EffectParams {
    std::unordered_map<std::string, float> scalars;
    std::unordered_map<std::string, Color> colors;
    std::unordered_map<std::string, std::string> strings;
    std::unordered_map<std::string, const SurfaceRGBA*> textures;
    std::unordered_map<std::string, const SurfaceRGBA*> aux_surfaces;
};

} // namespace tachyon::renderer2d