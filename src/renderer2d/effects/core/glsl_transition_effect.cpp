#include "tachyon/renderer2d/effects/core/glsl_transition_effect.h"
#include "tachyon/renderer2d/effects/core/effect_utils.h"
#include "tachyon/renderer2d/effects/core/transitions/transition_utils.h"
#include "tachyon/renderer2d/effects/core/transitions/basic_transitions.h"
#include "tachyon/renderer2d/effects/core/transitions/light_leak_transitions.h"
#include "tachyon/renderer2d/effects/core/transitions/artistic_transitions.h"
#include "tachyon/renderer2d/effects/diagnostics/diagnostic_surface.h"
#include "tachyon/core/transition/transition_effect_resolver.h"
#include "tachyon/core/transition/transition_descriptor.h"
#include "tachyon/core/policy/engine_policy.h"
#include "tachyon/core/animation/easing.h"

#include <algorithm>
#include <cmath>
#include <mutex>

namespace tachyon::renderer2d {

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4244)
#endif

namespace {

float transition_progress(const EffectParams& params) {
    float raw_t = clamp01(get_scalar(params, "t", get_scalar(params, "progress", 0.0f)));

    const auto preset_it = params.scalars.find("easing_preset");
    if (preset_it == params.scalars.end()) {
        return raw_t;
    }

    const int preset_val = static_cast<int>(preset_it->second);
    if (preset_val < 0 || preset_val > static_cast<int>(animation::EasingPreset::Custom)) {
        return raw_t;
    }

    const animation::EasingPreset preset = static_cast<animation::EasingPreset>(preset_val);
    animation::CubicBezierEasing bezier;

    if (preset == animation::EasingPreset::Custom) {
        bezier.cx1 = get_scalar(params, "bezier_cx1", 0.0f);
        bezier.cy1 = get_scalar(params, "bezier_cy1", 0.0f);
        bezier.cx2 = get_scalar(params, "bezier_cx2", 1.0f);
        bezier.cy2 = get_scalar(params, "bezier_cy2", 1.0f);
    }

    const double eased_t = animation::apply_easing(static_cast<double>(raw_t), preset, bezier);
    return static_cast<float>(clamp01(eased_t));
}

}  // namespace



SurfaceRGBA GlslTransitionEffect::apply(const SurfaceRGBA& input, const EffectParams& params) const {
    const float t = transition_progress(params);

    // Get transition ID from params
    std::string transition_id;
    const auto transition_it = params.strings.find("transition_id");
    if (transition_it != params.strings.end()) {
        transition_id = transition_it->second;
    }

    // Use resolver to resolve the transition effect
    TransitionEffectResolver resolver{m_registry};
    TransitionEffectRequest request;
    request.transition_id = transition_id;
    request.preferred_backend = TransitionRuntimeKind::CpuPixel;
    
    // Set fallback policy based on engine policy
    if (EngineValidationPolicy::instance().strict_transitions) {
        request.fallback_policy = TransitionFallbackPolicy::Strict;
    } else {
        request.fallback_policy = TransitionFallbackPolicy::Magenta;
    }
    
    auto resolved = resolver.resolve(request);
    
    // Handle invalid resolution based on fallback policy
    if (!resolved.valid) {
        switch (request.fallback_policy) {
            case TransitionFallbackPolicy::Strict:
                // In strict mode, return empty surface (caller should check)
                return SurfaceRGBA(input.width(), input.height());
                
            case TransitionFallbackPolicy::Magenta:
                return DiagnosticSurface::render(resolved, input.width(), input.height(), input.profile());
                
            case TransitionFallbackPolicy::NoOp:
                // Return input unchanged
                return input;
        }
    }

    const SurfaceRGBA* to_surface = nullptr;
    if (const auto to_it = params.aux_surfaces.find("transition_to"); to_it != params.aux_surfaces.end()) {
        to_surface = to_it->second;
    } else if (const auto to_fallback_it = params.aux_surfaces.find("to"); to_fallback_it != params.aux_surfaces.end()) {
        to_surface = to_fallback_it->second;
    } else if (const auto bg_it = params.aux_surfaces.find("background"); bg_it != params.aux_surfaces.end()) {
        to_surface = bg_it->second;
    }

    SurfaceRGBA output(input.width(), input.height());
    output.set_profile(input.profile());
    if (input.width() == 0U || input.height() == 0U) {
        return output;
    }

    if (resolved.kernel.valid && resolved.kernel.apply) {
        // Use resolved kernel (surface-level)
        output = resolved.kernel.apply(input, to_surface, t);
    } else {
        // Fallback: return input unchanged
        output = input;
    }

    return output;
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif

}  // namespace tachyon::renderer2d
