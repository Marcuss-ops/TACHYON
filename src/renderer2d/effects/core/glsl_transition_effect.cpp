#include "tachyon/renderer2d/effects/core/glsl_transition_effect.h"
#include "tachyon/renderer2d/effects/core/effect_utils.h"
#include "tachyon/renderer2d/effects/core/transitions/transition_utils.h"
#include "tachyon/renderer2d/effects/core/transitions/basic_transitions.h"
#include "tachyon/renderer2d/effects/core/transitions/light_leak_transitions.h"
#include "tachyon/renderer2d/effects/core/transitions/artistic_transitions.h"
#include "tachyon/transition_registry.h"
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

void register_builtin_transitions() {
    static std::once_flag flag;
    std::call_once(flag, []() {
        register_basic_transitions();
        register_light_leak_transitions();
        register_artistic_transitions();
    });
}

}  // namespace

void init_builtin_transitions() {
    register_builtin_transitions();
}

SurfaceRGBA GlslTransitionEffect::apply(const SurfaceRGBA& input, const EffectParams& params) const {
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

    #pragma omp parallel for schedule(static)
    for (int y = 0; y < static_cast<int>(input.height()); ++y) {
        for (std::uint32_t x = 0; x < input.width(); ++x) {
            const float u = (static_cast<float>(x) + 0.5f) / width;
            const float v = (static_cast<float>(y) + 0.5f) / height;

            Color out;
            if (transition_spec != nullptr && transition_spec->function != nullptr) {
                out = transition_spec->function(u, v, t, input, to_surface);
            } else {
                out = lerp_surface_color(input, to_surface, u, v, t);
            }

            output.set_pixel(x, static_cast<std::uint32_t>(y), out);
        }
    }

    return output;
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif

}  // namespace tachyon::renderer2d
