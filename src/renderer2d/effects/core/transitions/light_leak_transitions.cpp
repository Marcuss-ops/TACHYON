#include "tachyon/renderer2d/effects/core/transitions/light_leak_transitions.h"
#include "tachyon/renderer2d/effects/core/transitions/transition_utils.h"
#include "tachyon/transition_registry.h"
#include <cmath>
#include <algorithm>

namespace tachyon::renderer2d {

namespace {

Color transition_light_leak(float u, float v, float t, const SurfaceRGBA& input, const SurfaceRGBA* to_surface) {
    const bool overlay_mode = (to_surface == nullptr);
    const Color base = overlay_mode ? Color::black() : Color::lerp(sample_uv(input, u, v), sample_transition_target(input, to_surface, u, v), t);

    // 1. Primary Wide Beam (Amber/Gold/Siena)
    const float angle1 = -22.0f * 3.14159f / 180.0f;
    const float ca1 = std::cos(angle1);
    const float sa1 = std::sin(angle1);
    
    // Project with some organic curvature
    const float proj1 = (u * ca1 + v * sa1 + 0.1f + 0.05f * std::sin(v * 3.0f + t * 5.0f)) / 1.2f;
    const float pos1 = -0.6f + t * 2.2f;
    
    // Very wide beam with soft falloff for "morbidezza"
    const float width1 = 0.55f; 
    const float dist1 = std::abs(proj1 - pos1);
    const float edge1 = 1.0f - std::clamp(dist1 / width1, 0.0f, 1.0f);
    const float mask1 = edge1 * edge1 * (3.0f - 2.0f * edge1); // Cubic falloff for organic feel

    // 2. Secondary Glow (Deep Earthy Amber)
    const float width2 = 0.9f;
    const float edge2 = 1.0f - std::clamp(dist1 / width2, 0.0f, 1.0f);
    const float mask2 = std::pow(edge2, 4.0f);

    // Professional Amber/Siena Palette
    const Color siena = {0.627f, 0.322f, 0.176f, 1.0f};
    const Color deep_amber = {1.0f, 0.45f, 0.05f, 1.0f};
    const Color gold = {1.0f, 0.843f, 0.0f, 1.0f};
    const Color light_amber = {1.0f, 0.82f, 0.45f, 1.0f};
    
    // Evolving leak color
    Color leak_color = Color::lerp(siena, deep_amber, mask1);
    leak_color = Color::lerp(leak_color, gold, std::pow(mask1, 2.0f));
    
    // High-end highlights
    const float center_peak = std::pow(mask1, 6.0f);
    leak_color = Color::lerp(leak_color, {1.0f, 0.98f, 0.92f, 1.0f}, center_peak);

    // Organic flicker and shimmer
    const float noise = std::fmod(std::sin(t * 45.0f + u * 10.0f) * 43758.5453f, 0.06f);
    const float intensity = (1.8f * mask1 + 0.6f * mask2) * (0.95f + noise);

    return screen_over(base, leak_color, intensity);
}

Color transition_light_leak_solar(float u, float v, float t, const SurfaceRGBA& input, const SurfaceRGBA* to_surface) {
    const bool overlay_mode = (to_surface == nullptr);
    const Color base = overlay_mode ? Color::black() : Color::lerp(sample_uv(input, u, v), sample_transition_target(input, to_surface, u, v), t);
    const float angle = -10.0f * 3.14159f / 180.0f;
    const float proj = (u * std::cos(angle) + v * std::sin(angle) + 0.1f) / 1.2f;
    const float pos = -0.3f + t * 1.6f;
    const float mask = std::pow(std::max(0.0f, 1.0f - (std::abs(proj - pos) / 0.15f)), 3.0f);
    const Color solar = {1.0f, 0.98f, 0.8f, 1.0f};
    return screen_over(base, solar, 2.0f * mask * (0.9f + 0.1f * std::sin(t * 100.0f)));
}

Color transition_light_leak_nebula(float u, float v, float t, const SurfaceRGBA& input, const SurfaceRGBA* to_surface) {
    const bool overlay_mode = (to_surface == nullptr);
    const Color base = overlay_mode ? Color::black() : Color::lerp(sample_uv(input, u, v), sample_transition_target(input, to_surface, u, v), t);
    const float proj = (u * 0.707f + v * 0.707f + 0.4f) / 1.8f;
    const float pos = -0.5f + t * 2.0f;
    const float mask = std::max(0.0f, 1.0f - (std::abs(proj - pos) / 0.5f));
    Color nebula = Color::lerp({0.2f, 0.0f, 1.0f, 1.0f}, {0.6f, 0.0f, 0.8f, 1.0f}, u);
    return screen_over(base, nebula, 1.2f * mask * mask);
}

Color transition_light_leak_sunset(float u, float v, float t, const SurfaceRGBA& input, const SurfaceRGBA* to_surface) {
    const bool overlay_mode = (to_surface == nullptr);
    const Color base = overlay_mode ? Color::black() : Color::lerp(sample_uv(input, u, v), sample_transition_target(input, to_surface, u, v), t);
    const float proj1 = (u * 0.96f - v * 0.25f + 0.2f) / 1.4f;
    const float proj2 = (u * 0.96f + v * 0.25f + 0.1f) / 1.4f;
    const float pos = -0.3f + t * 1.6f;
    const float m1 = std::max(0.0f, 1.0f - (std::abs(proj1 - pos) / 0.2f));
    const float m2 = std::max(0.0f, 1.0f - (std::abs(proj2 - pos) / 0.2f));
    const Color sunset = Color::lerp({1.0f, 0.1f, 0.0f, 1.0f}, {1.0f, 0.6f, 0.0f, 1.0f}, t);
    return screen_over(base, sunset, 1.5f * (m1*m1 + m2*m2));
}

Color transition_light_leak_ghost(float u, float v, float t, const SurfaceRGBA& input, const SurfaceRGBA* to_surface) {
    const bool overlay_mode = (to_surface == nullptr);
    const Color base = overlay_mode ? Color::black() : Color::lerp(sample_uv(input, u, v), sample_transition_target(input, to_surface, u, v), t);
    const float proj = (u + v * 0.1f) / 1.1f;
    const float pos = -0.2f + t * 1.4f;
    const float mask = std::pow(std::max(0.0f, 1.0f - (std::abs(proj - pos) / 0.4f)), 4.0f);
    const Color ghost = {0.8f, 0.9f, 1.0f, 0.5f};
    return screen_over(base, ghost, mask * 0.7f);
}

Color transition_film_burn(float u, float v, float t, const SurfaceRGBA& input, const SurfaceRGBA* to_surface) {
    const bool overlay_mode = (to_surface == nullptr);
    const Color base = overlay_mode ? Color::black() : Color::lerp(sample_uv(input, u, v), sample_transition_target(input, to_surface, u, v), t);

    const float angle = -22.6f * 3.14159f / 180.0f;
    const float ca = std::cos(angle);
    const float sa = std::sin(angle);
    const float proj = (u * ca + v * sa + 0.2f) / 1.4f;

    const float pos = -0.3f + t * 1.6f;
    const float width = 0.12f;
    const float d = std::abs(proj - pos);
    float intensity = std::max(0.0f, 1.0f - (d / width));

    const Color ca_col = {1.0f, 0.32f, 0.0f, 1.0f};
    const Color cb_col = {1.0f, 0.667f, 0.137f, 1.0f};
    const Color burn = Color::lerp(ca_col, cb_col, std::clamp(proj, 0.0f, 1.0f));

    const float jitter = std::fmod(std::sin((u * 123.4f + v * 456.7f) * 12.9898f) * 43758.5453f, 0.15f);
    intensity = std::clamp(1.15f * intensity * intensity + (jitter * intensity), 0.0f, 1.2f);
    return screen_over(base, burn, intensity);
}

} // namespace

void register_light_leak_transitions() {
    auto& reg = TransitionRegistry::instance();
    const auto register_builtin = [&reg](const char* canonical_id,
                                         const char* name,
                                         const char* description,
                                         TransitionSpec::TransitionFn fn) {
        reg.register_transition({canonical_id, name, description, fn});
    };

    register_builtin("tachyon.transition.light_leak", "Light Leak", "High-quality evolving cinematic light leak", transition_light_leak);
    register_builtin("tachyon.transition.light_leak_solar", "Light Leak Solar", "Bright golden solar leak", transition_light_leak_solar);
    register_builtin("tachyon.transition.light_leak_nebula", "Light Leak Nebula", "Deep blue space nebula leak", transition_light_leak_nebula);
    register_builtin("tachyon.transition.light_leak_sunset", "Light Leak Sunset", "Warm dual-beam sunset leak", transition_light_leak_sunset);
    register_builtin("tachyon.transition.light_leak_ghost", "Light Leak Ghost", "Pale ethereal ghost leak", transition_light_leak_ghost);
    register_builtin("tachyon.transition.film_burn", "Film Burn", "Fiery red-orange film burn", transition_film_burn);
}

} // namespace tachyon::renderer2d
