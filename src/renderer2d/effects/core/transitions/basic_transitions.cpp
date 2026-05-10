#include "tachyon/renderer2d/effects/core/transitions/basic_transitions.h"
#include "tachyon/transition_registry.h"
#include "tachyon/renderer2d/effects/core/transitions/transition_utils.h"
#include "tachyon/core/transition/transition_descriptor.h"
#include "tachyon/core/ids/builtin_ids.h"
#include <cmath>
#include <algorithm>
#include "tachyon/renderer2d/effects/core/transitions/transition_fast_paths.h"

namespace tachyon::renderer2d {
// External fallbacks
Color transition_flash(float u, float v, float t, const SurfaceRGBA& input, const SurfaceRGBA* to_surface);
Color transition_zoom_blur(float u, float v, float t, const SurfaceRGBA& input, const SurfaceRGBA* to_surface);

namespace {

Color transition_crossfade(float u, float v, float t, const SurfaceRGBA& input, const SurfaceRGBA* to_surface) {
    const Color a = sample_uv(input, u, v);
    const Color b = sample_transition_target(input, to_surface, u, v);
    return Color::lerp(a, b, t);
}

Color transition_slide(float u, float v, float t, const SurfaceRGBA& input, const SurfaceRGBA* to_surface) {
    const Color source = sample_uv(input, u - t, v);
    const Color target = sample_transition_target(input, to_surface, u - t + 1.0f, v);
    return Color::lerp(source, target, t);
}

Color transition_slide_up(float u, float v, float t, const SurfaceRGBA& input, const SurfaceRGBA* to_surface) {
    const Color source = sample_uv(input, u, v + t);
    const Color target = sample_transition_target(input, to_surface, u, v + t - 1.0f);
    return Color::lerp(source, target, t);
}

Color transition_swipe_left(float u, float v, float t, const SurfaceRGBA& input, const SurfaceRGBA* to_surface) {
    const Color source = sample_uv(input, u + t, v);
    const Color target = sample_transition_target(input, to_surface, u + t - 1.0f, v);
    return Color::lerp(source, target, t);
}

Color transition_zoom(float u, float v, float t, const SurfaceRGBA& input, const SurfaceRGBA* to_surface) {
    const float zoom = 1.0f + t;
    const float su = 0.5f + (u - 0.5f) / zoom;
    const float sv = 0.5f + (v - 0.5f) / zoom;
    return Color::lerp(sample_uv(input, u, v), sample_transition_target(input, to_surface, su, sv), t);
}

Color transition_flip(float u, float v, float t, const SurfaceRGBA& input, const SurfaceRGBA* to_surface) {
    const float fu = (u < 0.5f) ? u * 2.0f : (1.0f - u) * 2.0f;
    return Color::lerp(sample_uv(input, fu, v), sample_transition_target(input, to_surface, u, v), t);
}

Color transition_blur(float u, float v, float t, const SurfaceRGBA& input, const SurfaceRGBA* to_surface) {
    const float radius = t * 0.1f;
    Color acc = Color::transparent();
    constexpr int samples = 5;
    for (int i = 0; i < samples; ++i) {
        const float offset = (static_cast<float>(i) / static_cast<float>(samples - 1) - 0.5f) * radius;
        acc = Color::lerp(acc, sample_uv(input, u + offset, v), 1.0f / static_cast<float>(samples));
    }
    return Color::lerp(acc, sample_transition_target(input, to_surface, u, v), t);
}

Color transition_fade_to_black(float u, float v, float t, const SurfaceRGBA& input, const SurfaceRGBA* to_surface) {
    const Color black = Color::black();
    if (t < 0.5f) {
        return Color::lerp(sample_uv(input, u, v), black, t * 2.0f);
    }
    const float tt = (t - 0.5f) * 2.0f;
    return Color::lerp(black, sample_transition_target(input, to_surface, u, v), tt);
}

Color transition_wipe_linear(float u, float v, float t, const SurfaceRGBA& input, const SurfaceRGBA* to_surface) {
    return (u < t) ? sample_transition_target(input, to_surface, u, v) : sample_uv(input, u, v);
}

Color transition_wipe_angular(float u, float v, float t, const SurfaceRGBA& input, const SurfaceRGBA* to_surface) {
    const float angle = t * 6.28318530718f;
    const float ux = u - 0.5f;
    const float vy = v - 0.5f;
    const float a = std::atan2(vy, ux) + 3.14159265359f;
    return (a < angle) ? sample_transition_target(input, to_surface, u, v) : sample_uv(input, u, v);
}

Color transition_push_left(float u, float v, float t, const SurfaceRGBA& input, const SurfaceRGBA* to_surface) {
    const Color source = sample_uv(input, u + t, v);
    const Color target = sample_transition_target(input, to_surface, u + t - 1.0f, v);
    return Color::lerp(source, target, t);
}

Color transition_slide_easing(float u, float v, float t, const SurfaceRGBA& input, const SurfaceRGBA* to_surface) {
    const float eased = t * t * (3.0f - 2.0f * t);
    const Color source = sample_uv(input, u + eased, v);
    const Color target = sample_transition_target(input, to_surface, u + eased - 1.0f, v);
    return Color::lerp(source, target, eased);
}

Color transition_circle_iris(float u, float v, float t, const SurfaceRGBA& input, const SurfaceRGBA* to_surface) {
    const float radius = std::sqrt((u - 0.5f) * (u - 0.5f) + (v - 0.5f) * (v - 0.5f));
    return (radius < t * 0.75f) ? sample_transition_target(input, to_surface, u, v) : sample_uv(input, u, v);
}

float smoothstep01(float x) {
    x = std::clamp(x, 0.0f, 1.0f);
    return x * x * (3.0f - 2.0f * x);
}

Color transition_smooth_wipe(float u, float v, float t, const SurfaceRGBA& input, const SurfaceRGBA* to_surface) {
    const float feather = 0.08f;
    const float edge = smoothstep01(t);
    const float mask = smoothstep01((edge - u + feather) / std::max(feather, 0.0001f));

    const Color a = sample_uv(input, u, v);
    const Color b = sample_transition_target(input, to_surface, u, v);

    return Color::lerp(a, b, mask);
}

Color transition_flash_cut(float u, float v, float t, const SurfaceRGBA& input, const SurfaceRGBA* to_surface) {
    const Color a = sample_uv(input, u, v);
    const Color b = sample_transition_target(input, to_surface, u, v);

    const Color base = Color::lerp(a, b, smoothstep01(t));

    const float flash = 1.0f - std::abs(t - 0.5f) * 2.0f;
    const float flash_amount = smoothstep01(flash) * 0.85f;

    Color white = Color::white();
    white.a = 1.0f;

    return Color::lerp(base, white, flash_amount);
}

Color transition_soft_zoom_blur(float u, float v, float t, const SurfaceRGBA& input, const SurfaceRGBA* to_surface) {
    constexpr int samples = 7;

    const float eased = smoothstep01(t);
    const float blur_phase = 0.12f * std::sin(eased * 3.1415926535f);
    const float cx = 0.5f;
    const float cy = 0.5f;

    Color acc = Color::transparent();

    for (int i = 0; i < samples; ++i) {
        const float s = static_cast<float>(i) / static_cast<float>(samples - 1);
        const float blur = (s - 0.5f) * blur_phase;

        const float su = cx + (u - cx) * (1.0f + blur);
        const float sv = cy + (v - cy) * (1.0f + blur);

        const Color src = sample_uv(input, su, sv);
        const Color dst = sample_transition_target(input, to_surface, su, sv);
        const Color mixed = Color::lerp(src, dst, eased);

        acc = Color::lerp(acc, mixed, 1.0f / static_cast<float>(i + 1));
    }

    return acc;
}

} // namespace

void resolve_basic_transition_implementations(tachyon::TransitionDescriptor& d) {
    using namespace tachyon;

    if (d.id == ids::transition::crossfade) d.cpu_fn = transition_crossfade;
    else if (d.id == ids::transition::slide_up) d.cpu_fn = transition_slide_up;
    else if (d.id == ids::transition::swipe_left) d.cpu_fn = transition_swipe_left;
    else if (d.id == ids::transition::zoom) {
        d.cpu_fn = transition_zoom;
        // Zoom doesn't have a fused direct kernel yet, but we can add one or use the generic path
    }
    else if (d.id == ids::transition::flip) d.cpu_fn = transition_flip;
    else if (d.id == ids::transition::blur) d.cpu_fn = transition_blur;
    else if (d.id == ids::transition::fade_to_black) d.cpu_fn = transition_fade_to_black;
    else if (d.id == ids::transition::wipe_linear) {
        d.cpu_fn = transition_wipe_linear;
        d.direct_cpu_fn = apply_wipe_linear_fused_direct;
    }
    else if (d.id == ids::transition::wipe_angular) d.cpu_fn = transition_wipe_angular;
    else if (d.id == ids::transition::push_left) d.cpu_fn = transition_push_left;
    else if (d.id == ids::transition::slide_easing) d.cpu_fn = transition_slide_easing;
    else if (d.id == ids::transition::circle_iris || d.id == "iris") {
        d.cpu_fn = transition_circle_iris;
        d.direct_cpu_fn = apply_circle_iris_fused_direct;
    }
    else if (d.id == ids::transition::flash_cut) {
        d.cpu_fn = transition_flash;
        d.direct_cpu_fn = apply_flash_cut_fused_direct;
    }
    else if (d.id == ids::transition::soft_zoom_blur) {
        d.cpu_fn = transition_zoom_blur;
        d.direct_cpu_fn = apply_soft_zoom_blur_fused_direct;
    }
    else if (d.id == ids::transition::smooth_wipe) {
        d.cpu_fn = transition_smooth_wipe;
        d.direct_cpu_fn = apply_smooth_wipe_fused_direct;
    }
    else if (d.id == ids::transition::slide) {
        d.cpu_fn = transition_slide;
        d.direct_cpu_fn = apply_slide_fused_direct;
    }
}

} // namespace tachyon::renderer2d
