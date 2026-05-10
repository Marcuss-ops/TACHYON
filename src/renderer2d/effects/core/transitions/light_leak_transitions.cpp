#include "tachyon/renderer2d/effects/core/transitions/light_leak_transitions.h"
#include "tachyon/transition_registry.h"
#include "tachyon/renderer2d/effects/core/transitions/transition_utils.h"
#include "tachyon/core/transition/transition_descriptor.h"
#include "tachyon/core/ids/builtin_ids.h"
#include <cmath>
#include <algorithm>

namespace tachyon::renderer2d {

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

    float flicker_amount;
    float pulse_amount;

    enum class Shape {
        Edge,
        Sweep,
        RadialCorner,
        HorizontalBand,
        WindowBands,
        DualBeam,
        Prism,
        Blobs
    } shape;
};

namespace {

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
    float center = 0.5f + (s.direction * 0.1f) * std::sin(t * 2.0f);
    return soft_band(v, center, s.width, s.softness);
}

float window_bands_mask(float u, float v, float t, const LightLeakStyle& s) {
    float drift = 0.03f * std::sin(t * 2.0f);
    float m1 = soft_band(u, 0.20f + drift, s.width, s.softness);
    float m2 = soft_band(u, 0.48f + drift, s.width, s.softness);
    float m3 = soft_band(u, 0.76f + drift, s.width, s.softness);
    return std::clamp(m1 + m2 * 0.7f + m3 * 0.5f, 0.0f, 1.0f);
}

float dual_beam_mask(float u, float v, float t, const LightLeakStyle& s) {
    float a1 = radians(s.angle_degrees);
    float a2 = radians(s.angle_degrees + 20.0f);
    float proj1 = u * std::cos(a1) + v * std::sin(a1);
    float proj2 = u * std::cos(a2) - v * std::sin(a2);
    float center = -s.offset + t * s.speed;
    return 0.7f * soft_band(proj1, center, s.width, s.softness) + 
           0.5f * soft_band(proj2, center * 0.9f, s.width * 0.8f, s.softness);
}

float prism_mask(float u, float v, float t, const LightLeakStyle& s) {
    float proj = u * 0.8f + v * 0.2f;
    float center = -s.offset + t * s.speed;
    float m1 = soft_band(proj, center - 0.05f, s.width * 0.5f, s.softness);
    float m2 = soft_band(proj, center,         s.width * 0.5f, s.softness);
    float m3 = soft_band(proj, center + 0.05f, s.width * 0.5f, s.softness);
    return std::clamp(m1 + m2 + m3, 0.0f, 1.0f);
}

float blobs_mask(float u, float v, float t, const LightLeakStyle& s) {
    auto field = [&](float cx, float cy, float w) {
        float dx = u - cx;
        float dy = v - cy;
        float d2 = dx * dx + dy * dy;
        return w * std::exp(-d2 * 3.0f); // Replaced 12.0f with 3.0f for massive wide blobs
    };
    
    const float phase = t * 2.8f * s.speed; 
    float m = 0.0f;
    m += field(0.5f + 0.40f * std::cos(phase + 0.3f), 0.5f + 0.30f * std::sin(phase * 0.9f), 2.2f);
    m += field(0.3f + 0.35f * std::sin(phase * 1.3f), 0.3f + 0.35f * std::cos(phase * 0.8f + 0.4f), 1.8f);
    m += field(0.7f + 0.25f * std::cos(phase * 1.1f), 0.6f + 0.35f * std::sin(phase * 1.2f), 2.0f);
    m += field(0.5f + 0.45f * std::sin(phase * 0.7f), 0.8f + 0.25f * std::cos(phase * 1.4f), 1.6f);

    const float dist_mid = t - 0.5f;
    const float boost = 1.0f + 4.0f * std::exp(-(dist_mid * dist_mid) / 0.06f); 
    
    return std::pow(std::clamp(m * boost * 0.3f, 0.0f, 1.0f), s.softness);
}

float evaluate_light_leak_mask(float u, float v, float t, const LightLeakStyle& style) {
    switch (style.shape) {
        case LightLeakStyle::Shape::Edge:           return edge_mask(u, v, t, style);
        case LightLeakStyle::Shape::Sweep:          return sweep_mask(u, v, t, style);
        case LightLeakStyle::Shape::RadialCorner:   return radial_corner_mask(u, v, t, style);
        case LightLeakStyle::Shape::HorizontalBand: return horizontal_band_mask(u, v, t, style);
        case LightLeakStyle::Shape::WindowBands:    return window_bands_mask(u, v, t, style);
        case LightLeakStyle::Shape::DualBeam:       return dual_beam_mask(u, v, t, style);
        case LightLeakStyle::Shape::Prism:          return prism_mask(u, v, t, style);
        case LightLeakStyle::Shape::Blobs:          return blobs_mask(u, v, t, style);
        default: return 0.0f;
    }
}

} // namespace

Color apply_light_leak_style(
    float u,
    float v,
    float t,
    const SurfaceRGBA& input,
    const SurfaceRGBA* to_surface,
    const LightLeakStyle& style
) {
    float mask = 0.0f;
    const bool overlay_mode = (to_surface == nullptr);
    Color base;
    // --- CINEMATIC TRANSITION LOGIC ---
    // Fast but smooth transition to avoid "dirty" color mixing while preventing pops
    const float color_t = smoothstep01(std::clamp((t - 0.2f) / 0.6f, 0.0f, 1.0f));

    if (overlay_mode) {
        base = sample_uv(input, u, v);
    } else {
        base = Color::lerp(
            sample_uv(input, u, v),
            sample_transition_target(input, to_surface, u, v),
            color_t
        );
    }

    // Optimization: Cache frame-constant values to avoid re-calculating trig/noise for millions of pixels
    static thread_local float last_t = -1.0f;
    static thread_local const char* last_style_id = nullptr;
    static thread_local float cached_cos_a, cached_sin_a, cached_center, cached_flicker;

    if (t != last_t || style.id != last_style_id) {
        std::cout << "[KERNEL DEBUG] apply_light_leak_style fired for " << style.id << " at t=" << t << std::endl;
        const float angle_rad = radians(style.angle_degrees);
        cached_cos_a = std::cos(angle_rad);
        cached_sin_a = std::sin(angle_rad);
        cached_center = -style.offset + t * style.speed;
        // Smooth temporal flicker instead of frame-by-frame random jumps to maintain continuity
        const float continuous_flicker = 0.5f + 0.35f * std::sin(t * 41.0f) + 0.15f * std::sin(t * 97.0f);
        cached_flicker = 1.0f + style.flicker_amount * (continuous_flicker - 0.5f);
        last_t = t;
        last_style_id = style.id;
    }

    // Inline the most common shape (Sweep) for performance, fallback for others
    if (style.shape == LightLeakStyle::Shape::Sweep) {
        float proj = u * cached_cos_a + v * cached_sin_a;
        mask = soft_band(proj, cached_center, style.width, style.softness);
    } else {
        mask = evaluate_light_leak_mask(u, v, t, style);
    }
    
    // Trust style softness: enforce linear mask falloff to isolate the sweep
    // and prevent background pollution across the entire frame.
    float intensity = style.intensity * mask * cached_flicker * 5.0f;

    // Pulse effect
    if (style.pulse_amount > 0.0f) {
        intensity *= (1.0f + style.pulse_amount * std::sin(t * 12.0f));
    }

    Color leak = Color::lerp(style.color_a, style.color_b, mask);
    
    // Sharper highlight
    leak = Color::lerp(leak, style.highlight, mask * mask * mask);

    // --- ABSOLUTE SATURATION ENGINE ---
    // Replicates the high-contrast 'Black Render' environment on ANY background.
    // Dims the background beneath the leak mask, allowing pure additive saturation
    // to define the visual output, preventing washout on white/light scenes.
    float hole_factor = smoothstep01(std::clamp(mask / 0.4f, 0.0f, 1.0f));
    base.r *= (1.0f - hole_factor);
    base.g *= (1.0f - hole_factor);
    base.b *= (1.0f - hole_factor);

    // Clean raw intensity now that blending is corrected
    float final_intensity = intensity; 

    // Custom additive blend: respect style structs
    Color result;
    result.r = std::min(base.r + leak.r * final_intensity, 1.0f);
    result.g = std::min(base.g + leak.g * final_intensity, 1.0f);
    result.b = std::min(base.b + leak.b * final_intensity, 1.0f);
    
    // Maintain background alpha
    result.a = base.a;

    return result;
}

// --- Style Definitions ---

// 1. Classic Light Leak
static constexpr LightLeakStyle kClassicLeak{
    "tachyon.transition.light_leak", "Classic Leak", "Wide amber cinematic leak",
    {1.0f, 0.45f, 0.05f, 1.0f}, {1.0f, 0.78f, 0.30f, 1.0f}, {1.0f, 0.98f, 0.92f, 1.0f},
    -22.0f, 0.55f, 2.0f, 1.8f, 2.2f, 0.6f, -1.0f, 0.05f, 0.0f, LightLeakStyle::Shape::Sweep
};

// 2. Solar Flare
static constexpr LightLeakStyle kSolarFlare{
    "tachyon.transition.light_leak_solar", "Solar Flare", "Bright golden solar flare",
    {1.0f, 0.9f, 0.2f, 1.0f}, {1.0f, 1.0f, 0.6f, 1.0f}, {1.0f, 1.0f, 1.0f, 1.0f},
    45.0f, 0.40f, 1.5f, 3.0f, 1.8f, 0.4f, 1.0f, 0.08f, 0.1f, LightLeakStyle::Shape::RadialCorner
};

// 3. Blue Nebula
static constexpr LightLeakStyle kBlueNebula{
    "tachyon.transition.light_leak_nebula", "Blue Nebula", "Cosmic blue and purple leak",
    {0.0f, 0.3f, 1.0f, 1.0f}, {0.6f, 0.2f, 1.0f, 1.0f}, {0.8f, 0.9f, 1.0f, 1.0f},
    0.0f, 0.60f, 2.5f, 2.2f, 1.2f, 0.2f, -1.0f, 0.02f, 0.05f, LightLeakStyle::Shape::RadialCorner
};

// 4. Sunset Dual
static constexpr LightLeakStyle kSunsetDual{
    "tachyon.transition.light_leak_sunset", "Sunset Dual", "Warm dual-beam sunset leak",
    {1.0f, 0.15f, 0.0f, 1.0f}, {1.0f, 0.55f, 0.1f, 1.0f}, {1.0f, 0.95f, 0.6f, 1.0f},
    15.0f, 0.45f, 2.2f, 3.5f, 2.0f, 0.5f, 1.0f, 0.05f, 0.0f, LightLeakStyle::Shape::DualBeam
};

// 5. Pale Ghost
static constexpr LightLeakStyle kPaleGhost{
    "tachyon.transition.light_leak_ghost", "Pale Ghost", "Ethereal pale white leak",
    {0.8f, 0.85f, 1.0f, 1.0f}, {0.9f, 0.95f, 1.0f, 1.0f}, {1.0f, 1.0f, 1.0f, 1.0f},
    -5.0f, 0.65f, 3.0f, 1.4f, 1.5f, 0.4f, -1.0f, 0.02f, 0.15f, LightLeakStyle::Shape::Sweep
};

// 6. Film Burn
static constexpr LightLeakStyle kFilmBurn{
    "tachyon.transition.film_burn", "Film Burn", "Classic fiery film burn",
    {1.0f, 0.2f, 0.0f, 1.0f}, {1.0f, 0.6f, 0.1f, 1.0f}, {1.0f, 0.9f, 0.2f, 1.0f},
    -20.0f, 0.20f, 1.2f, 4.0f, 2.5f, 0.3f, -1.0f, 0.15f, 0.0f, LightLeakStyle::Shape::Sweep
};

// 7. Soft Warm Edge
static constexpr LightLeakStyle kSoftWarmEdge{
    "tachyon.transition.lightleak.soft_warm_edge", "Soft Warm Edge", "Gentle warm edge leak",
    {1.0f, 0.3f, 0.1f, 1.0f}, {1.0f, 0.6f, 0.2f, 1.0f}, {1.0f, 0.9f, 0.7f, 1.0f},
    0.0f, 0.50f, 2.0f, 1.2f, 1.0f, 0.0f, -1.0f, 0.04f, 0.0f, LightLeakStyle::Shape::Edge
};

// 8. Golden Sweep (Saturated for Additive)
static constexpr LightLeakStyle kGoldenSweep{
    "tachyon.transition.lightleak.golden_sweep", "Golden Sweep", "Premium golden sweep",
    {1.0f, 0.7f, 0.0f, 1.0f}, {1.0f, 0.9f, 0.2f, 1.0f}, {1.0f, 1.0f, 0.5f, 1.0f},
    25.0f, 0.65f, 1.5f, 4.0f, 2.0f, 0.4f, 1.0f, 0.12f, 0.0f, LightLeakStyle::Shape::Sweep
};

// 9. Creamy White
static constexpr LightLeakStyle kCreamyWhite{
    "tachyon.transition.lightleak.creamy_white", "Creamy White", "Soft creamy white leak",
    {1.0f, 0.98f, 0.95f, 1.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 1.0f, 1.0f},
    0.0f, 0.30f, 1.8f, 1.5f, 1.2f, 0.0f, 1.0f, 0.02f, 0.0f, LightLeakStyle::Shape::HorizontalBand
};

// 10. Dusty Archive
static constexpr LightLeakStyle kDustyArchive{
    "tachyon.transition.lightleak.dusty_archive", "Dusty Archive", "Textured vintage leak",
    {0.7f, 0.4f, 0.2f, 1.0f}, {0.9f, 0.6f, 0.3f, 1.0f}, {1.0f, 0.9f, 0.7f, 1.0f},
    -35.0f, 0.18f, 2.0f, 1.8f, 1.4f, 0.3f, 1.0f, 0.12f, 0.0f, LightLeakStyle::Shape::WindowBands
};

// 11. Lens Flare Pass
static constexpr LightLeakStyle kLensFlarePass{
    "tachyon.transition.lightleak.lens_flare_pass", "Lens Flare Pass", "Fast blue flare sweep",
    {0.2f, 0.6f, 1.0f, 1.0f}, {0.6f, 0.8f, 1.0f, 1.0f}, {1.0f, 1.0f, 1.0f, 1.0f},
    15.0f, 0.04f, 1.2f, 3.0f, 4.0f, 1.8f, 1.0f, 0.05f, 0.0f, LightLeakStyle::Shape::Sweep
};

// 12. Amber Multi-Sweep (Fire Amber - High Saturation)
static constexpr LightLeakStyle kAmberSweep{
    "tachyon.transition.lightleak.amber_sweep", "Amber Sweep", "Rich fiery amber sweep",
    {1.0f, 0.2f, 0.0f, 1.0f}, {1.0f, 0.4f, 0.0f, 1.0f}, {1.0f, 0.6f, 0.0f, 1.0f},
    -15.0f, 0.85f, 0.4f, 4.0f, 2.2f, 0.3f, 1.0f, 0.10f, 0.0f, LightLeakStyle::Shape::Sweep
};


// 13. Neon Pulse
static constexpr LightLeakStyle kNeonPulse{
    "tachyon.transition.lightleak.neon_pulse", "Neon Pulse", "Futuristic neon pink leak",
    {1.0f, 0.0f, 0.6f, 1.0f}, {0.4f, 0.0f, 1.0f, 1.0f}, {1.0f, 0.8f, 1.0f, 1.0f},
    30.0f, 0.30f, 1.5f, 2.5f, 2.0f, 0.5f, 1.0f, 0.10f, 0.3f, LightLeakStyle::Shape::Sweep
};

// 14. Prism Shatter
static constexpr LightLeakStyle kPrismShatter{
    "tachyon.transition.lightleak.prism_shatter", "Prism Shatter", "Rainbow refractive prism",
    {1.0f, 0.2f, 0.2f, 1.0f}, {0.2f, 1.0f, 0.2f, 1.0f}, {0.2f, 0.2f, 1.0f, 1.0f},
    45.0f, 0.10f, 1.0f, 2.0f, 2.5f, 0.6f, 1.0f, 0.05f, 0.0f, LightLeakStyle::Shape::Prism
};

// 15. Vintage Sepia
static constexpr LightLeakStyle kVintageSepia{
    "tachyon.transition.lightleak.vintage_sepia", "Vintage Sepia", "Warm sepia memory leak",
    {0.4f, 0.2f, 0.1f, 1.0f}, {0.7f, 0.5f, 0.3f, 1.0f}, {0.9f, 0.8f, 0.6f, 1.0f},
    -10.0f, 0.80f, 3.5f, 1.2f, 1.0f, 0.2f, -1.0f, 0.15f, 0.0f, LightLeakStyle::Shape::Edge
};

// 16. Organic Blobs
static constexpr LightLeakStyle kOrganicBlobs{
    "tachyon.transition.lightleak.organic_blobs", "Organic Blobs", "Fluid Gaussian light blobs",
    {1.0f, 0.3f, 0.6f, 1.0f}, {1.0f, 0.6f, 0.1f, 1.0f}, {1.0f, 1.0f, 0.8f, 1.0f},
    0.0f, 1.0f, 1.0f, 4.5f, 1.3f, 0.0f, 1.0f, 0.05f, 0.1f, LightLeakStyle::Shape::Blobs
};

// --- Wrapper functions ---

#define DEFINE_LEAK_WRAPPER(name, style) \
Color transition_##name(float u, float v, float t, const SurfaceRGBA& input, const SurfaceRGBA* to_surface) { \
    return apply_light_leak_style(u, v, t, input, to_surface, style); \
}

DEFINE_LEAK_WRAPPER(light_leak, kClassicLeak)
DEFINE_LEAK_WRAPPER(light_leak_solar, kSolarFlare)
DEFINE_LEAK_WRAPPER(light_leak_nebula, kBlueNebula)
DEFINE_LEAK_WRAPPER(light_leak_sunset, kSunsetDual)
DEFINE_LEAK_WRAPPER(light_leak_ghost, kPaleGhost)
DEFINE_LEAK_WRAPPER(film_burn, kFilmBurn)

DEFINE_LEAK_WRAPPER(lightleak_soft_warm_edge, kSoftWarmEdge)
DEFINE_LEAK_WRAPPER(lightleak_golden_sweep, kGoldenSweep)
DEFINE_LEAK_WRAPPER(lightleak_creamy_white, kCreamyWhite)
DEFINE_LEAK_WRAPPER(lightleak_dusty_archive, kDustyArchive)
DEFINE_LEAK_WRAPPER(lightleak_lens_flare_pass, kLensFlarePass)
DEFINE_LEAK_WRAPPER(lightleak_amber_sweep, kAmberSweep)
DEFINE_LEAK_WRAPPER(lightleak_neon_pulse, kNeonPulse)
DEFINE_LEAK_WRAPPER(lightleak_prism_shatter, kPrismShatter)
DEFINE_LEAK_WRAPPER(lightleak_vintage_sepia, kVintageSepia)
DEFINE_LEAK_WRAPPER(lightleak_organic_blobs, kOrganicBlobs)

void resolve_light_leak_implementations(tachyon::TransitionDescriptor& d) {
    using namespace tachyon;

    if (d.id == ids::transition::light_leak) d.cpu_fn = transition_light_leak;
    else if (d.id == ids::transition::light_leak_solar) d.cpu_fn = transition_light_leak_solar;
    else if (d.id == ids::transition::light_leak_nebula) d.cpu_fn = transition_light_leak_nebula;
    else if (d.id == ids::transition::light_leak_sunset) d.cpu_fn = transition_light_leak_sunset;
    else if (d.id == ids::transition::light_leak_ghost) d.cpu_fn = transition_lightleak_organic_blobs;
    else if (d.id == ids::transition::film_burn) d.cpu_fn = transition_film_burn;
    else if (d.id == ids::transition::lightleak_soft_warm_edge) d.cpu_fn = transition_lightleak_soft_warm_edge;
    else if (d.id == ids::transition::lightleak_golden_sweep) d.cpu_fn = transition_lightleak_golden_sweep;
    else if (d.id == ids::transition::lightleak_creamy_white) d.cpu_fn = transition_lightleak_creamy_white;
    else if (d.id == ids::transition::lightleak_dusty_archive) d.cpu_fn = transition_lightleak_dusty_archive;
    else if (d.id == ids::transition::lightleak_lens_flare_pass) d.cpu_fn = transition_lightleak_lens_flare_pass;
    else if (d.id == ids::transition::lightleak_amber_sweep) d.cpu_fn = transition_lightleak_amber_sweep;
    else if (d.id == ids::transition::lightleak_neon_pulse) d.cpu_fn = transition_lightleak_neon_pulse;
    else if (d.id == ids::transition::lightleak_prism_shatter) d.cpu_fn = transition_lightleak_prism_shatter;
    else if (d.id == ids::transition::lightleak_vintage_sepia) d.cpu_fn = transition_lightleak_vintage_sepia;
    else if (d.id == ids::transition::lightleak_organic_blobs) d.cpu_fn = transition_lightleak_organic_blobs;
}

} // namespace tachyon::renderer2d
