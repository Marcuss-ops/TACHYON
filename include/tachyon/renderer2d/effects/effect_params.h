#pragma once

#include "tachyon/renderer2d/core/framebuffer.h"
#include <string>
#include <unordered_map>

namespace tachyon::renderer2d {

struct RenderContext2D;

/**
 * @brief Runtime parameters for an effect.
 */
struct EffectParams {
    const RenderContext2D* context{nullptr};
    std::unordered_map<std::string, float> scalars;
    std::unordered_map<std::string, Color> colors;
    std::unordered_map<std::string, std::string> strings;
    std::unordered_map<std::string, const SurfaceRGBA*> textures;
    std::unordered_map<std::string, const SurfaceRGBA*> aux_surfaces;
};

} // namespace tachyon::renderer2d