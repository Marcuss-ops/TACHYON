#include "tachyon/renderer2d/effects/core/transitions/artistic_transitions.h"
#include "tachyon/transition_registry.h"
#include "tachyon/renderer2d/effects/core/transitions/transition_utils.h"
#include "tachyon/core/transition/transition_descriptor.h"
#include "tachyon/core/ids/builtin_ids.h"
#include <cmath>
#include <algorithm>

namespace tachyon::renderer2d {

namespace {

Color transition_zoom_in(float u, float v, float t, const SurfaceRGBA& input, const SurfaceRGBA* to_surface) {
    const float zoom = std::max(0.001f, 1.0f - t);
    const float su = 0.5f + (u - 0.5f) / zoom;
    const float sv = 0.5f + (v - 0.5f) / zoom;
    const Color source = sample_uv(input, u, v);
    const Color target = sample_transition_target(input, to_surface, su, sv);
    return Color::lerp(source, target, t);
}

} // namespace

Color transition_zoom_blur(float u, float v, float t, const SurfaceRGBA& input, const SurfaceRGBA* to_surface) {
    const float zoom = std::max(0.001f, 1.0f - t);
    const float su = 0.5f + (u - 0.5f) / zoom;
    const float sv = 0.5f + (v - 0.5f) / zoom;
    Color acc = Color::transparent();
    constexpr int samples = 6;
    for (int i = 0; i < samples; ++i) {
        const float s = static_cast<float>(i) / static_cast<float>(samples - 1);
        const float offset = (s - 0.5f) * t * 0.12f;
        acc = Color::lerp(acc, sample_transition_target(input, to_surface, su + offset, sv + offset), 1.0f / static_cast<float>(samples));
    }
    return Color::lerp(sample_uv(input, u, v), acc, t);
}

namespace {

Color transition_spin(float u, float v, float t, const SurfaceRGBA& input, const SurfaceRGBA* to_surface) {
    const float angle = t * 6.28318530718f;
    const float dx = u - 0.5f;
    const float dy = v - 0.5f;
    const float c = std::cos(angle);
    const float s = std::sin(angle);
    const float ru = 0.5f + dx * c - dy * s;
    const float rv = 0.5f + dx * s + dy * c;
    const Color source = sample_uv(input, ru, rv);
    const Color target = sample_transition_target(input, to_surface, ru, rv);
    return Color::lerp(source, target, t);
}

Color transition_pixelate(float u, float v, float t, const SurfaceRGBA& input, const SurfaceRGBA* to_surface) {
    const float grid = std::max(2.0f, 20.0f - 19.0f * t);
    const float pu = std::floor(u * grid) / grid;
    const float pv = std::floor(v * grid) / grid;
    const Color source = sample_uv(input, pu, pv);
    const Color target = sample_transition_target(input, to_surface, pu, pv);
    return Color::lerp(source, target, t);
}

Color transition_glitch_slice(float u, float v, float t, const SurfaceRGBA& input, const SurfaceRGBA* to_surface) {
    const float band = std::floor(v * 10.0f);
    const float jitter = std::sin(band * 12.0f + t * 10.0f) * 0.02f;
    const Color source = sample_uv(input, u + jitter * 0.5f, v);
    const Color target = sample_transition_target(input, to_surface, u + jitter, v);
    return Color::lerp(source, target, t);
}

Color transition_rgb_split(float u, float v, float t, const SurfaceRGBA& input, const SurfaceRGBA* to_surface) {
    const Color source = sample_uv(input, u, v);
    const Color target = {
        sample_transition_target(input, to_surface, u + 0.01f * t, v).r,
        sample_transition_target(input, to_surface, u, v).g,
        sample_transition_target(input, to_surface, u - 0.01f * t, v).b,
        sample_transition_target(input, to_surface, u, v).a
    };
    return Color::lerp(source, target, t);
}

Color transition_luma_dissolve(float u, float v, float t, const SurfaceRGBA& input, const SurfaceRGBA* to_surface) {
    const float noise = std::fmod(std::sin((u * 123.4f + v * 456.7f) * 12.9898f) * 43758.5453f, 1.0f);
    return (noise < t) ? sample_transition_target(input, to_surface, u, v) : sample_uv(input, u, v);
}

Color transition_directional_blur_wipe(float u, float v, float t, const SurfaceRGBA& input, const SurfaceRGBA* to_surface) {
    const float slide = std::clamp(t * 1.5f, 0.0f, 1.0f);
    const float sigma = 0.01f + 0.04f * t;
    Color acc = Color::transparent();
    constexpr int samples = 5;
    for (int i = 0; i < samples; ++i) {
        const float s = static_cast<float>(i) / static_cast<float>(samples - 1);
        const float off = (s - 0.5f) * sigma;
        acc = Color::lerp(acc, sample_transition_target(input, to_surface, u + slide + off, v), 1.0f / static_cast<float>(samples));
    }
    return Color::lerp(sample_uv(input, u, v), acc, t);
}

} // namespace

Color transition_flash(float u, float v, float t, const SurfaceRGBA& input, const SurfaceRGBA* to_surface) {
    const Color a = sample_uv(input, u, v);
    const Color b = sample_transition_target(input, to_surface, u, v);

    float flash = 0.0f;
    if (t < 0.5f) {
        flash = t * 2.0f;
    } else {
        flash = (1.0f - t) * 2.0f;
    }

    const Color base = Color::lerp(a, b, t);
    const Color flash_color = {1.0f, 1.0f, 1.0f, 1.0f};
    return Color::lerp(base, flash_color, flash * 0.8f);
}

namespace {

Color transition_kaleidoscope(float u, float v, float t, const SurfaceRGBA& input, const SurfaceRGBA* to_surface) {
    const float dx = u - 0.5f;
    const float dy = v - 0.5f;
    const float radius = std::sqrt(dx * dx + dy * dy);
    const float angle = std::atan2(dy, dx);
    
    const float sides = 6.0f;
    const float segment = 6.28318530718f / sides;
    float local_angle = std::fmod(angle, segment);
    if (local_angle < 0.0f) local_angle += segment;
    if (local_angle > segment * 0.5f) local_angle = segment - local_angle;
    
    const float zoom = 1.0f - t * 0.5f;
    const float ru = 0.5f + std::cos(local_angle) * radius * zoom;
    const float rv = 0.5f + std::sin(local_angle) * radius * zoom;
    
    const Color a = sample_uv(input, ru, rv);
    const Color b = sample_transition_target(input, to_surface, u, v);
    return Color::lerp(a, b, t);
}

Color transition_ripple(float u, float v, float t, const SurfaceRGBA& input, const SurfaceRGBA* to_surface) {
    const float dx = u - 0.5f;
    const float dy = v - 0.5f;
    const float dist = std::sqrt(dx * dx + dy * dy);
    
    const float wave = std::sin(dist * 30.0f - t * 20.0f) * 0.03f * (1.0f - t);
    const float ru = u + wave * (dx / dist);
    const float rv = v + wave * (dy / dist);
    
    const Color a = sample_uv(input, ru, rv);
    const Color b = sample_transition_target(input, to_surface, u, v);
    return Color::lerp(a, b, t);
}

} // namespace

void resolve_artistic_transition_implementations(tachyon::TransitionDescriptor& d) {
    using namespace tachyon;

    if (d.id == ids::transition::zoom_in) d.cpu_fn = transition_zoom_in;
    else if (d.id == ids::transition::zoom_blur) d.cpu_fn = transition_zoom_blur;
    else if (d.id == ids::transition::spin) d.cpu_fn = transition_spin;
    else if (d.id == ids::transition::pixelate) d.cpu_fn = transition_pixelate;
    else if (d.id == ids::transition::glitch_slice) d.cpu_fn = transition_glitch_slice;
    else if (d.id == ids::transition::rgb_split) d.cpu_fn = transition_rgb_split;
    else if (d.id == ids::transition::luma_dissolve) d.cpu_fn = transition_luma_dissolve;
    else if (d.id == ids::transition::directional_blur_wipe) d.cpu_fn = transition_directional_blur_wipe;
    else if (d.id == ids::transition::flash) d.cpu_fn = transition_flash;
    else if (d.id == ids::transition::kaleidoscope) d.cpu_fn = transition_kaleidoscope;
    else if (d.id == ids::transition::ripple) d.cpu_fn = transition_ripple;
}

} // namespace tachyon::renderer2d
