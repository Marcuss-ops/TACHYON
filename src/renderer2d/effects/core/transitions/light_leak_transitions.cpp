#include "tachyon/renderer2d/effects/core/transitions/light_leak_transitions.h"
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
        Prism
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

float evaluate_light_leak_mask(float u, float v, float t, const LightLeakStyle& style) {
    switch (style.shape) {
        case LightLeakStyle::Shape::Edge:           return edge_mask(u, v, t, style);
        case LightLeakStyle::Shape::Sweep:          return sweep_mask(u, v, t, style);
        case LightLeakStyle::Shape::RadialCorner:   return radial_corner_mask(u, v, t, style);
        case LightLeakStyle::Shape::HorizontalBand: return horizontal_band_mask(u, v, t, style);
        case LightLeakStyle::Shape::WindowBands:    return window_bands_mask(u, v, t, style);
        case LightLeakStyle::Shape::DualBeam:       return dual_beam_mask(u, v, t, style);
        case LightLeakStyle::Shape::Prism:          return prism_mask(u, v, t, style);
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
    const bool overlay_mode = (to_surface == nullptr);
    const float eased_t = smoothstep01(t);

    Color base = overlay_mode
        ? sample_uv(input, u, v)
        : Color::lerp(
            sample_uv(input, u, v),
            sample_transition_target(input, to_surface, u, v),
            eased_t
        );

    float mask = evaluate_light_leak_mask(u, v, t, style);
    float flicker = 1.0f + style.flicker_amount * (cheap_noise(u, v, t * 10.0f) - 0.5f);
    float intensity = style.intensity * mask * flicker;

    // Pulse effect
    if (style.pulse_amount > 0.0f) {
        intensity *= (1.0f + style.pulse_amount * std::sin(t * 12.0f));
    }

    Color leak = Color::lerp(style.color_a, style.color_b, mask);
    leak = Color::lerp(leak, style.highlight, std::pow(mask, 3.0f));

    return screen_over(base, leak, intensity);
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

// 8. Golden Sweep
static constexpr LightLeakStyle kGoldenSweep{
    "tachyon.transition.lightleak.golden_sweep", "Golden Sweep", "Premium golden sweep",
    {1.0f, 0.8f, 0.1f, 1.0f}, {1.0f, 0.95f, 0.5f, 1.0f}, {1.0f, 1.0f, 0.9f, 1.0f},
    90.0f, 0.35f, 2.5f, 2.0f, 1.6f, 0.5f, 1.0f, 0.03f, 0.0f, LightLeakStyle::Shape::Sweep
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

// 12. Amber Multi-Sweep (Refactored to be compatible)
static constexpr LightLeakStyle kAmberSweep{
    "tachyon.transition.lightleak.amber_sweep", "Amber Sweep", "Dynamic amber sweep",
    {1.0f, 0.5f, 0.1f, 1.0f}, {1.0f, 0.8f, 0.3f, 1.0f}, {1.0f, 1.0f, 0.9f, 1.0f},
    -15.0f, 0.40f, 2.0f, 1.6f, 1.8f, 0.4f, 1.0f, 0.05f, 0.0f, LightLeakStyle::Shape::Sweep
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

void register_light_leak_implementations() {
    using namespace tachyon;

    {
        TransitionDescriptor d;
        d.id = std::string(ids::transition::light_leak);
        d.display_name = "Classic Leak";
        d.description = "Wide amber cinematic leak";
        d.runtime_kind = TransitionRuntimeKind::CpuPixel;
        d.category = TransitionKind::Fade;
        d.cpu_fn = transition_light_leak;
        d.capabilities = {.supports_cpu = true};
        d.params = registry::ParameterSchema({});
        register_transition_descriptor(d);
    }
    {
        TransitionDescriptor d;
        d.id = std::string(ids::transition::light_leak_solar);
        d.display_name = "Solar Flare";
        d.description = "Bright golden solar flare";
        d.runtime_kind = TransitionRuntimeKind::CpuPixel;
        d.category = TransitionKind::Fade;
        d.cpu_fn = transition_light_leak_solar;
        d.capabilities = {.supports_cpu = true};
        d.params = registry::ParameterSchema({});
        register_transition_descriptor(d);
    }
    {
        TransitionDescriptor d;
        d.id = std::string(ids::transition::light_leak_nebula);
        d.display_name = "Blue Nebula";
        d.description = "Cosmic blue and purple leak";
        d.runtime_kind = TransitionRuntimeKind::CpuPixel;
        d.category = TransitionKind::Fade;
        d.cpu_fn = transition_light_leak_nebula;
        d.capabilities = {.supports_cpu = true};
        d.params = registry::ParameterSchema({});
        register_transition_descriptor(d);
    }
    {
        TransitionDescriptor d;
        d.id = std::string(ids::transition::light_leak_sunset);
        d.display_name = "Sunset Dual";
        d.description = "Warm dual-beam sunset leak";
        d.runtime_kind = TransitionRuntimeKind::CpuPixel;
        d.category = TransitionKind::Fade;
        d.cpu_fn = transition_light_leak_sunset;
        d.capabilities = {.supports_cpu = true};
        d.params = registry::ParameterSchema({});
        register_transition_descriptor(d);
    }
    {
        TransitionDescriptor d;
        d.id = std::string(ids::transition::light_leak_ghost);
        d.display_name = "Pale Ghost";
        d.description = "Ethereal pale white leak";
        d.runtime_kind = TransitionRuntimeKind::CpuPixel;
        d.category = TransitionKind::Fade;
        d.cpu_fn = transition_light_leak_ghost;
        d.capabilities = {.supports_cpu = true};
        d.params = registry::ParameterSchema({});
        register_transition_descriptor(d);
    }
    {
        TransitionDescriptor d;
        d.id = std::string(ids::transition::film_burn);
        d.display_name = "Film Burn";
        d.description = "Classic fiery film burn";
        d.runtime_kind = TransitionRuntimeKind::CpuPixel;
        d.category = TransitionKind::Fade;
        d.cpu_fn = transition_film_burn;
        d.capabilities = {.supports_cpu = true};
        d.params = registry::ParameterSchema({});
        register_transition_descriptor(d);
    }
    {
        TransitionDescriptor d;
        d.id = std::string(ids::transition::lightleak_soft_warm_edge);
        d.display_name = "Soft Warm Edge";
        d.description = "Gentle warm edge leak";
        d.runtime_kind = TransitionRuntimeKind::CpuPixel;
        d.category = TransitionKind::Fade;
        d.cpu_fn = transition_lightleak_soft_warm_edge;
        d.capabilities = {.supports_cpu = true};
        d.params = registry::ParameterSchema({});
        register_transition_descriptor(d);
    }
    {
        TransitionDescriptor d;
        d.id = std::string(ids::transition::lightleak_golden_sweep);
        d.display_name = "Golden Sweep";
        d.description = "Premium golden sweep";
        d.runtime_kind = TransitionRuntimeKind::CpuPixel;
        d.category = TransitionKind::Fade;
        d.cpu_fn = transition_lightleak_golden_sweep;
        d.capabilities = {.supports_cpu = true};
        d.params = registry::ParameterSchema({});
        register_transition_descriptor(d);
    }
    {
        TransitionDescriptor d;
        d.id = std::string(ids::transition::lightleak_creamy_white);
        d.display_name = "Creamy White";
        d.description = "Soft creamy white leak";
        d.runtime_kind = TransitionRuntimeKind::CpuPixel;
        d.category = TransitionKind::Fade;
        d.cpu_fn = transition_lightleak_creamy_white;
        d.capabilities = {.supports_cpu = true};
        d.params = registry::ParameterSchema({});
        register_transition_descriptor(d);
    }
    {
        TransitionDescriptor d;
        d.id = std::string(ids::transition::lightleak_dusty_archive);
        d.display_name = "Dusty Archive";
        d.description = "Textured vintage leak";
        d.runtime_kind = TransitionRuntimeKind::CpuPixel;
        d.category = TransitionKind::Fade;
        d.cpu_fn = transition_lightleak_dusty_archive;
        d.capabilities = {.supports_cpu = true};
        d.params = registry::ParameterSchema({});
        register_transition_descriptor(d);
    }
    {
        TransitionDescriptor d;
        d.id = std::string(ids::transition::lightleak_lens_flare_pass);
        d.display_name = "Lens Flare Pass";
        d.description = "Fast blue flare sweep";
        d.runtime_kind = TransitionRuntimeKind::CpuPixel;
        d.category = TransitionKind::Fade;
        d.cpu_fn = transition_lightleak_lens_flare_pass;
        d.capabilities = {.supports_cpu = true};
        d.params = registry::ParameterSchema({});
        register_transition_descriptor(d);
    }
    {
        TransitionDescriptor d;
        d.id = std::string(ids::transition::lightleak_amber_sweep);
        d.display_name = "Amber Sweep";
        d.description = "Dynamic amber sweep";
        d.runtime_kind = TransitionRuntimeKind::CpuPixel;
        d.category = TransitionKind::Fade;
        d.cpu_fn = transition_lightleak_amber_sweep;
        d.capabilities = {.supports_cpu = true};
        d.params = registry::ParameterSchema({});
        register_transition_descriptor(d);
    }
    {
        TransitionDescriptor d;
        d.id = std::string(ids::transition::lightleak_neon_pulse);
        d.display_name = "Neon Pulse";
        d.description = "Futuristic neon pink leak";
        d.runtime_kind = TransitionRuntimeKind::CpuPixel;
        d.category = TransitionKind::Fade;
        d.cpu_fn = transition_lightleak_neon_pulse;
        d.capabilities = {.supports_cpu = true};
        d.params = registry::ParameterSchema({});
        register_transition_descriptor(d);
    }
    {
        TransitionDescriptor d;
        d.id = std::string(ids::transition::lightleak_prism_shatter);
        d.display_name = "Prism Shatter";
        d.description = "Rainbow refractive prism";
        d.runtime_kind = TransitionRuntimeKind::CpuPixel;
        d.category = TransitionKind::Fade;
        d.cpu_fn = transition_lightleak_prism_shatter;
        d.capabilities = {.supports_cpu = true};
        d.params = registry::ParameterSchema({});
        register_transition_descriptor(d);
    }
    {
        TransitionDescriptor d;
        d.id = std::string(ids::transition::lightleak_vintage_sepia);
        d.display_name = "Vintage Sepia";
        d.description = "Warm sepia memory leak";
        d.runtime_kind = TransitionRuntimeKind::CpuPixel;
        d.category = TransitionKind::Fade;
        d.cpu_fn = transition_lightleak_vintage_sepia;
        d.capabilities = {.supports_cpu = true};
        d.params = registry::ParameterSchema({});
        register_transition_descriptor(d);
    }
}

} // namespace tachyon::renderer2d
