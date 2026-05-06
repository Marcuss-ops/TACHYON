#include "tachyon/renderer2d/effects/core/transitions/light_leak_transitions.h"
#include "tachyon/renderer2d/effects/core/transitions/transition_utils.h"
#include "tachyon/transition_registry.h"
#include <cmath>
#include <algorithm>
#include <iostream>

namespace tachyon::renderer2d {

namespace {

struct LightLeakStyle {
    const char* id;
    const char* name;
    const char* description;

    Color color_a;
    Color color_b;
    Color highlight;

    float angle_degrees;
    float width;
    float softness;
    float intensity;
    float speed;
    float offset;
    float direction; // -1 for left/top, 1 for right/bottom

    float grain_amount;
    float flicker_amount;
    float pulse_amount;

    enum class Shape {
        Edge,
        Sweep,
        RadialCorner,
        HorizontalBand,
        WindowBands,
        ProjectorCone,
        Prism
    } shape;
};

float smoothstep01(float x) {
    x = std::clamp(x, 0.0f, 1.0f);
    return x * x * (3.0f - 2.0f * x);
}

float radians(float degrees) {
    return degrees * 3.1415926535f / 180.0f;
}

float cheap_noise(float u, float v, float t) {
    const float n = std::sin((u * 12.9898f + v * 78.233f + t * 37.719f) * 43758.5453f);
    return n - std::floor(n);
}

float soft_band(float value, float center, float width, float softness) {
    const float d = std::abs(value - center);
    float x = 1.0f - std::clamp(d / std::max(width, 0.0001f), 0.0f, 1.0f);
    x = smoothstep01(x);
    return std::pow(x, std::max(softness, 0.1f));
}

float edge_mask(float u, float v, float t, const LightLeakStyle& s) {
    float side = s.direction < 0 ? u : 1.0f - u;
    float drift = 0.08f * std::sin(t * 3.0f + v * 2.0f);
    return std::pow(1.0f - std::clamp((side + drift) / s.width, 0.0f, 1.0f), s.softness);
}

float sweep_mask(float u, float v, float t, const LightLeakStyle& s) {
    float a = radians(s.angle_degrees);
    float proj = u * std::cos(a) + v * std::sin(a);
    float center = -s.offset + t * s.speed;
    return soft_band(proj, center, s.width, s.softness);
}

float radial_corner_mask(float u, float v, float t, const LightLeakStyle& s) {
    float dx = (s.direction < 0) ? u : 1.0f - u;
    float dy = (s.direction < 0) ? v : 1.0f - v;
    float d = std::sqrt(dx * dx + dy * dy);
    float pulse = 1.0f + s.pulse_amount * std::sin(t * 3.14159f);
    return std::pow(1.0f - std::clamp(d / (s.width * pulse), 0.0f, 1.0f), s.softness);
}

float horizontal_band_mask(float u, float v, float t, const LightLeakStyle& s) {
    float center = 0.60f - 0.15f * smoothstep01(t);
    return soft_band(v, center, s.width, s.softness);
}

float window_bands_mask(float u, float v, float t, const LightLeakStyle& s) {
    float drift = 0.03f * std::sin(t * 2.0f);
    float m1 = soft_band(u, 0.20f + drift, s.width, s.softness);
    float m2 = soft_band(u, 0.48f + drift, s.width, s.softness);
    float m3 = soft_band(u, 0.76f + drift, s.width, s.softness);
    return std::clamp(m1 + m2 * 0.7f + m3 * 0.5f, 0.0f, 1.0f);
}

// Blob gaussiano morbido in coordinate normalizzate
float gaussian_blob(float u, float v, float cx, float cy, float radius) {
    const float dx = u - cx;
    const float dy = v - cy;
    const float rr = std::max(radius * radius, 0.000001f);
    const float d2 = dx * dx + dy * dy;
    return std::exp(-0.5f * d2 / rr);
}

float evaluate_light_leak_mask(float u, float v, float t, const LightLeakStyle& style) {
    switch (style.shape) {
        case LightLeakStyle::Shape::Edge: return edge_mask(u, v, t, style);
        case LightLeakStyle::Shape::Sweep: return sweep_mask(u, v, t, style);
        case LightLeakStyle::Shape::RadialCorner: return radial_corner_mask(u, v, t, style);
        case LightLeakStyle::Shape::HorizontalBand: return horizontal_band_mask(u, v, t, style);
        case LightLeakStyle::Shape::WindowBands: return window_bands_mask(u, v, t, style);
        default: return 0.0f;
    }
}

Color apply_light_leak_style(
    float u,
    float v,
    float t,
    const SurfaceRGBA& input,
    const SurfaceRGBA* to_surface,
    const LightLeakStyle& style
) {
    const bool overlay_mode = (to_surface == nullptr);

    Color base = overlay_mode
        ? Color::black()
        : Color::lerp(
            sample_uv(input, u, v),
            sample_transition_target(input, to_surface, u, v),
            smoothstep01(t)
        );

    float mask = evaluate_light_leak_mask(u, v, t, style);

    float flicker = 1.0f + style.flicker_amount * cheap_noise(u, v, t);
    float intensity = style.intensity * mask * flicker;

    Color leak = Color::lerp(style.color_a, style.color_b, mask);
    leak = Color::lerp(leak, style.highlight, std::pow(mask, 4.0f));

    return screen_over(base, leak, intensity);
}

// Premium Styles Definitions
static constexpr LightLeakStyle kSoftWarmEdge{
    "tachyon.transition.lightleak.soft_warm_edge",
    "Soft Warm Edge Leak",
    "Premium warm edge light leak",
    {1.0f, 0.25f, 0.05f, 1.0f}, // Deeper orange
    {1.0f, 0.54f, 0.18f, 1.0f},
    {1.0f, 0.95f, 0.72f, 1.0f},
    -10.0f, // angle
    0.50f,  // width
    1.8f,   // softness
    0.95f,  // intensity (very high)
    1.15f,  // speed
    0.35f,  // offset
    -1.0f,  // direction (left)
    0.02f,  // grain
    0.05f,  // flicker
    0.0f,   // pulse
    LightLeakStyle::Shape::Edge
};

static constexpr LightLeakStyle kGoldenSweep{
    "tachyon.transition.lightleak.golden_sweep",
    "Golden Sweep",
    "Soft golden cinematic sweep",
    {1.0f, 0.85f, 0.12f, 1.0f}, // Very gold
    {1.0f, 1.0f, 0.60f, 1.0f},
    {1.0f, 1.0f, 1.0f, 1.0f},
    90.0f,  // angle (Vertical!)
    0.40f,  // width
    2.0f,   // softness
    1.20f,  // intensity
    1.5f,   // speed
    0.50f,  // offset
    1.0f,   // direction
    0.01f,  // grain
    0.02f,  // flicker
    0.0f,   // pulse
    LightLeakStyle::Shape::Sweep
};

static constexpr LightLeakStyle kCreamyWhite{
    "tachyon.transition.lightleak.creamy_white",
    "Creamy White Leak",
    "Soft warm white memory leak",
    {1.0f, 1.0f, 1.0f, 1.0f}, // Pure White
    {1.0f, 0.95f, 0.90f, 1.0f},
    {1.0f, 1.0f, 1.0f, 1.0f},
    0.0f,   // angle
    0.25f,  // width
    1.5f,   // softness
    1.50f,  // intensity
    1.0f,   // speed
    0.0f,   // offset
    1.0f,   // direction
    0.00f,  // grain
    0.01f,  // flicker
    0.0f,   // pulse
    LightLeakStyle::Shape::HorizontalBand // Different Shape!
};

static constexpr LightLeakStyle kDustyArchive{
    "tachyon.transition.lightleak.dusty_archive",
    "Dusty Archive Leak",
    "Warm archival light leak with subtle grain",
    {0.82f, 0.38f, 0.12f, 1.0f},
    {1.0f, 0.62f, 0.22f, 1.0f},
    {1.0f, 0.90f, 0.68f, 1.0f},
    -45.0f, // angle (different angle)
    0.15f,  // width (thinner bands)
    2.5f,   // softness
    0.90f,  // intensity (increased)
    1.1f,   // speed
    0.25f,  // offset
    1.0f,   // direction
    0.12f,  // grain (increased)
    0.08f,  // flicker
    0.0f,   // pulse
    LightLeakStyle::Shape::WindowBands // Changed shape for variety
};

static constexpr LightLeakStyle kLensFlarePass{
    "tachyon.transition.lightleak.lens_flare_pass",
    "Subtle Lens Flare Pass",
    "Thin premium lens flare sweep",
    {0.42f, 0.74f, 1.0f, 1.0f}, // Bluer
    {0.82f, 0.92f, 1.0f, 1.0f},
    {1.0f, 1.0f, 1.0f, 1.0f},
    15.0f,   // angle
    0.035f,  // width (very thin)
    1.5f,    // softness
    1.40f,   // intensity (bright flare)
    3.5f,    // speed (fast)
    1.55f,   // offset
    1.0f,    // direction
    0.00f,   // grain
    0.05f,   // flicker
    0.0f,    // pulse
    LightLeakStyle::Shape::Sweep
};

// Wrapper functions
Color transition_lightleak_soft_warm_edge(float u, float v, float t, const SurfaceRGBA& input, const SurfaceRGBA* to_surface) {
    return apply_light_leak_style(u, v, t, input, to_surface, kSoftWarmEdge);
}

Color transition_lightleak_golden_sweep(float u, float v, float t, const SurfaceRGBA& input, const SurfaceRGBA* to_surface) {
    return apply_light_leak_style(u, v, t, input, to_surface, kGoldenSweep);
}

Color transition_lightleak_creamy_white(float u, float v, float t, const SurfaceRGBA& input, const SurfaceRGBA* to_surface) {
    return apply_light_leak_style(u, v, t, input, to_surface, kCreamyWhite);
}

Color transition_lightleak_dusty_archive(float u, float v, float t, const SurfaceRGBA& input, const SurfaceRGBA* to_surface) {
    return apply_light_leak_style(u, v, t, input, to_surface, kDustyArchive);
}

Color transition_lightleak_lens_flare_pass(float u, float v, float t, const SurfaceRGBA& input, const SurfaceRGBA* to_surface) {
    return apply_light_leak_style(u, v, t, input, to_surface, kLensFlarePass);
}

// Versione Tachyon della tua idea "light leak amber".
// 4 blob caldi che attraversano lo schermo da sinistra a destra.
Color transition_lightleak_amber_sweep(float u, float v, float t,
                                       const SurfaceRGBA& input,
                                       const SurfaceRGBA* to_surface) {
    // Smooth traversal
    const float eased = 0.5f - 0.5f * std::cos(std::clamp(t, 0.0f, 1.0f) * 3.14159265f);

    // Base crossfade A/B
    const bool overlay_mode = (to_surface == nullptr);
    Color base = overlay_mode 
        ? sample_uv(input, u, v) 
        : Color::lerp(sample_uv(input, u, v), sample_transition_target(input, to_surface, u, v), eased);

    float acc = 0.0f;
    constexpr int kBlobCount = 4;
    constexpr int kSeed = 3;

    for (int i = 0; i < kBlobCount; ++i) {
        const int s0 = kSeed + i * 17;

        // Attraversamento sinistra -> destra in coordinate normalizzate [0..1]
        const float start_x = -0.25f + 0.10f * cheap_noise(static_cast<float>(s0), 0.0f, 0.0f);
        const float end_x   =  1.25f - 0.10f * cheap_noise(static_cast<float>(s0 + 3), 0.0f, 0.0f);

        float cx = start_x + (end_x - start_x) * eased;
        cx += std::sin(t * 6.0f + static_cast<float>(i)) * 0.020f;

        float cy = 0.20f + 0.60f * cheap_noise(static_cast<float>(s0 + 1), 0.0f, 0.0f);
        cy += std::cos(t * 5.0f + static_cast<float>(i) * 1.3f) * 0.030f;

        const float radius = 0.30f;

        // Blob principale
        float blob = gaussian_blob(u, v, cx, cy, radius);
        blob *= (0.80f + 0.20f * eased);
        blob *= (0.50f + static_cast<float>(i) * 0.10f);

        acc += blob;
    }

    const float leak_mask = std::clamp(acc * 0.22f, 0.0f, 1.0f);

    // Palette amber/gold/cream
    const Color amber = {1.0f, 0.48f, 0.12f, 1.0f};
    const Color gold  = {1.0f, 0.78f, 0.30f, 1.0f};
    const Color cream = {1.0f, 0.96f, 0.88f, 1.0f};

    Color leak_color = Color::lerp(amber, gold, std::clamp(leak_mask * 1.4f, 0.0f, 1.0f));
    leak_color = Color::lerp(leak_color, cream, std::pow(leak_mask, 3.0f));

    return screen_over(base, leak_color, leak_mask);
}


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
    
    // Different projection for solar
    const float proj = (u + v) * 0.5f;
    const float pos = -0.5f + t * 2.0f;
    const float mask = std::pow(std::max(0.0f, 1.0f - (std::abs(proj - pos) / 0.35f)), 2.0f);
    
    const Color solar = {1.0f, 1.0f, 0.4f, 1.0f}; // Very yellow/white
    return screen_over(base, solar, 3.5f * mask); // Very bright
}

Color transition_light_leak_nebula(float u, float v, float t, const SurfaceRGBA& input, const SurfaceRGBA* to_surface) {
    const bool overlay_mode = (to_surface == nullptr);
    const Color base = overlay_mode ? Color::black() : Color::lerp(sample_uv(input, u, v), sample_transition_target(input, to_surface, u, v), t);
    
    // Radial nebula
    const float dx = u - 0.5f;
    const float dy = v - 0.5f;
    const float d = std::sqrt(dx*dx + dy*dy);
    const float radius = 0.2f + t * 0.8f;
    const float mask = std::max(0.0f, 1.0f - std::abs(d - radius) / 0.3f);
    
    Color nebula = Color::lerp({0.0f, 0.4f, 1.0f, 1.0f}, {0.8f, 0.2f, 1.0f, 1.0f}, t);
    return screen_over(base, nebula, 2.0f * mask * mask);
}

Color transition_light_leak_sunset(float u, float v, float t, const SurfaceRGBA& input, const SurfaceRGBA* to_surface) {
    const bool overlay_mode = (to_surface == nullptr);
    const Color base = overlay_mode ? Color::black() : Color::lerp(sample_uv(input, u, v), sample_transition_target(input, to_surface, u, v), t);
    
    const float proj1 = (u * 0.96f - v * 0.25f);
    const float proj2 = (u * 0.96f + v * 0.25f);
    const float pos = -0.5f + t * 2.0f;
    
    const float m1 = std::max(0.0f, 1.0f - (std::abs(proj1 - pos) / 0.45f));
    const float m2 = std::max(0.0f, 1.0f - (std::abs(proj2 - pos) / 0.45f));
    const Color sunset = Color::lerp({1.0f, 0.1f, 0.0f, 1.0f}, {1.0f, 0.6f, 0.0f, 1.0f}, t);
    return screen_over(base, sunset, 4.0f * (m1*m1 + m2*m2)); // Extremely bright
}

Color transition_light_leak_ghost(float u, float v, float t, const SurfaceRGBA& input, const SurfaceRGBA* to_surface) {
    const bool overlay_mode = (to_surface == nullptr);
    const Color base = overlay_mode ? Color::black() : Color::lerp(sample_uv(input, u, v), sample_transition_target(input, to_surface, u, v), t);
    const float proj = (u + v * 0.1f);
    const float pos = -0.4f + t * 1.8f;
    const float mask = std::pow(std::max(0.0f, 1.0f - (std::abs(proj - pos) / 0.6f)), 4.0f);
    const Color ghost = {0.8f, 0.9f, 1.0f, 1.0f}; // Increased opacity
    return screen_over(base, ghost, mask * 1.5f); // Increased intensity
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
    std::cout << "DEBUG: Registering light leak transitions..." << std::endl;
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

    // Premium Light Leaks
    register_builtin("tachyon.transition.lightleak.soft_warm_edge", "Soft Warm Edge Leak", "Premium warm edge light leak", transition_lightleak_soft_warm_edge);
    register_builtin("tachyon.transition.lightleak.golden_sweep", "Golden Sweep", "Soft golden cinematic sweep", transition_lightleak_golden_sweep);
    register_builtin("tachyon.transition.lightleak.creamy_white", "Creamy White Leak", "Soft warm white memory leak", transition_lightleak_creamy_white);
    register_builtin("tachyon.transition.lightleak.dusty_archive", "Dusty Archive Leak", "Warm archival light leak with subtle grain", transition_lightleak_dusty_archive);
    register_builtin("tachyon.transition.lightleak.lens_flare_pass", "Subtle Lens Flare Pass", "Thin premium lens flare sweep", transition_lightleak_lens_flare_pass);

    register_builtin("tachyon.transition.lightleak.amber_sweep",
                     "Amber Sweep",
                     "Warm multi-blob amber light leak sweeping left to right",
                     transition_lightleak_amber_sweep);
}

} // namespace tachyon::renderer2d
