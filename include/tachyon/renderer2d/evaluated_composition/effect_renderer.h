#pragma once

#include "tachyon/renderer2d/effects/effect_host.h"
#include "tachyon/renderer2d/resource/render_context.h"
#include "tachyon/core/spec/scene_spec.h"

#include <memory>
#include <vector>

namespace tachyon {
namespace renderer2d {

EffectHost& effect_host_for(RenderContext2D& context);

EffectParams effect_params_from_spec(const EffectSpec& spec);

SurfaceRGBA apply_effect_pipeline(
    const SurfaceRGBA& input,
    const std::vector<EffectSpec>& effects,
    EffectHost& host);

} // namespace renderer2d
} // namespace tachyon