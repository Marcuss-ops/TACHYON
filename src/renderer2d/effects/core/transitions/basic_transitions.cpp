#include "tachyon/renderer2d/effects/core/transitions/basic_transitions.h"
#include "tachyon/renderer2d/effects/core/transitions/transition_utils.h"
#include "tachyon/transition_registry.h"
#include <cmath>
#include <algorithm>

namespace tachyon::renderer2d {

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

} // namespace

void register_basic_transitions() {
    auto& reg = TransitionRegistry::instance();
    
    #define REGISTER_BASIC(name) reg.register_cpu_implementation("transition_" #name, transition_##name)
    
    REGISTER_BASIC(crossfade);
    REGISTER_BASIC(slide);
    REGISTER_BASIC(slide_up);
    REGISTER_BASIC(swipe_left);
    REGISTER_BASIC(zoom);
    REGISTER_BASIC(flip);
    REGISTER_BASIC(blur);
    REGISTER_BASIC(fade_to_black);
    REGISTER_BASIC(wipe_linear);
    REGISTER_BASIC(wipe_angular);
    REGISTER_BASIC(push_left);
    REGISTER_BASIC(slide_easing);
    REGISTER_BASIC(circle_iris);
}

} // namespace tachyon::renderer2d
