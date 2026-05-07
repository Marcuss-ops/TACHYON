#pragma once

#include "tachyon/renderer2d/effects/core/effect_host.h"
#include "tachyon/renderer2d/resource/render_context.h"
#include "tachyon/core/spec/schema/objects/scene_spec.h"
#include "tachyon/runtime/core/diagnostics/diagnostics.h"

#include <unordered_map>
#include <memory>
#include <vector>

namespace tachyon {
namespace renderer2d {

class EffectRegistry;

EffectHost& effect_host_for(RenderContext2D& context);

EffectParams effect_params_from_spec(const EffectSpec& spec, const ColorProfile& working_profile);
EffectParams effect_params_from_spec(const EffectSpec& spec);

ResolutionResult<SurfaceRGBA> apply_effect_pipeline(
    const SurfaceRGBA& input,
    const std::vector<EffectSpec>& effects,
    EffectHost& host,
    const ColorProfile& working_profile,
    FrameDiagnostics* diagnostics = nullptr,
    const std::string& current_layer_id = "");

ResolutionResult<SurfaceRGBA> apply_effect_pipeline(
    const SurfaceRGBA& input,
    const std::vector<EffectSpec>& effects,
    EffectHost& host,
    const ColorProfile& working_profile,
    const std::unordered_map<std::string, std::shared_ptr<SurfaceRGBA>>& surfaces,
    const std::string& current_layer_id,
    FrameDiagnostics* diagnostics = nullptr);

} // namespace renderer2d
} // namespace tachyon
