#include "tachyon/renderer2d/effects/core/glsl_transition_effect.h"
#include "tachyon/renderer2d/effects/core/effect_utils.h"
#include "tachyon/transition_registry.h"
#include "tachyon/core/animation/easing.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <mutex>

namespace tachyon::renderer2d {

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4244)
#endif

namespace {  // Local helpers

Color sample_uv(const SurfaceRGBA& surface, float u, float v) {
    return sample_texture_bilinear(surface, std::clamp(u, 0.0f, 1.0f), std::clamp(v, 0.0f, 1.0f), Color::white());
}

Color sample_transition_source(const SurfaceRGBA& input, const SurfaceRGBA* aux, float u, float v) {
    if (aux != nullptr) {
        return sample_uv(*aux, u, v);
    }
    return sample_uv(input, u, v);
}

[[maybe_unused]] std::string transition_id_from_params(const EffectParams& params) {
    if (const auto transition_it = params.strings.find("transition_id"); transition_it != params.strings.end()) {
        return transition_it->second;
    }
    return "tachyon.transition.crossfade";
}

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

Color lerp_surface_color(const SurfaceRGBA& a, const SurfaceRGBA* b, float u, float v, float t) {
    const Color ca = sample_uv(a, u, v);
    const Color cb = sample_transition_source(a, b, u, v);
    return Color::lerp(ca, cb, clamp01(t));
}

}  // namespace

void init_builtin_transitions() {
}

SurfaceRGBA GlslTransitionEffect::apply(const SurfaceRGBA& input, const EffectParams& params) const {
    init_builtin_transitions();
    
    const float t = transition_progress(params);
    
    const TransitionSpec* transition_spec = nullptr;
    const auto transition_it = params.strings.find("transition_id");
    if (transition_it != params.strings.end()) {
        transition_spec = TransitionRegistry::instance().find(transition_it->second);
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
    
    const float width = static_cast<float>(std::max<std::uint32_t>(1U, input.width()));
    const float height = static_cast<float>(std::max<std::uint32_t>(1U, input.height()));
    
    for (std::uint32_t y = 0; y < input.height(); ++y) {
        for (std::uint32_t x = 0; x < input.width(); ++x) {
            const float u = (static_cast<float>(x) + 0.5f) / width;
            const float v = (static_cast<float>(y) + 0.5f) / height;
            
            Color out;
            if (transition_spec != nullptr && transition_spec->function != nullptr) {
                out = transition_spec->function(u, v, t, input, to_surface);
            } else {
                out = lerp_surface_color(input, to_surface, u, v, t);
            }
            
            output.set_pixel(x, y, out);
        }
    }
    
    return output;
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif

}  // namespace tachyon::renderer2d
