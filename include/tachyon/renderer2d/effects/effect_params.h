#pragma once

#include "tachyon/renderer2d/core/framebuffer.h"
#include "tachyon/runtime/resource/render_context.h"
#include <string>
#include <unordered_map>
#include <variant>

namespace tachyon::renderer2d {

/**
 * @brief Unified parameter value for runtime effects.
 */
using EffectParameterValue = std::variant<
    float,
    Color,
    std::string,
    bool,
    math::Vector2,
    const SurfaceRGBA*
>;

/**
 * @brief Runtime parameters for an effect.
 */
struct EffectParams {
    const RenderContext* context{nullptr};
    std::unordered_map<std::string, EffectParameterValue> params;
};

} // namespace tachyon::renderer2d