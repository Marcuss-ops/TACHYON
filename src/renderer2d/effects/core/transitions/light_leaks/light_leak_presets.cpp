#include "light_leak_internal.h"
#include "tachyon/renderer2d/effects/core/transitions/light_leak_transitions.h"
#include "tachyon/transition_registry.h"
#include "tachyon/core/transition/transition_descriptor.h"
#include "tachyon/core/ids/builtin_ids.h"
#include <algorithm>
#include <array>
#include <string_view>

namespace tachyon::renderer2d {

// --- Modern Named Style Definitions (C++20 Designated Initializers) ---

inline constexpr LightLeakStyle kClassicLeak{
    .id = "tachyon.transition.light_leak",
    .name = "Classic Leak",
    .description = "Wide amber cinematic leak",
    .inner_color = {1.0f, 0.45f, 0.05f, 1.0f},
    .mid_color = {1.0f, 0.78f, 0.30f, 1.0f},
    .outer_color = {1.0f, 0.98f, 0.92f, 1.0f},
    .angle = -22.0f,
    .spread = 0.55f,
    .softness = 2.0f,
    .intensity = 1.8f,
    .speed = 2.2f,
    .offset = 0.6f,
    .direction = -1.0f,
    .grain = 0.05f,
    .pulse = 0.0f,
    .shape = LightLeakStyle::Shape::Sweep
};

inline constexpr LightLeakStyle kSolarFlare{
    .id = "tachyon.transition.light_leak_solar",
    .name = "Solar Flare",
    .description = "Bright golden solar flare",
    .inner_color = {1.0f, 0.9f, 0.2f, 1.0f},
    .mid_color = {1.0f, 1.0f, 0.6f, 1.0f},
    .outer_color = {1.0f, 1.0f, 1.0f, 1.0f},
    .angle = 45.0f,
    .spread = 0.40f,
    .softness = 1.5f,
    .intensity = 3.0f,
    .speed = 1.8f,
    .offset = 0.4f,
    .direction = 1.0f,
    .grain = 0.08f,
    .pulse = 0.1f,
    .shape = LightLeakStyle::Shape::RadialCorner
};

inline constexpr LightLeakStyle kBlueNebula{
    .id = "tachyon.transition.light_leak_nebula",
    .name = "Blue Nebula",
    .description = "Cosmic blue and purple leak",
    .inner_color = {0.0f, 0.3f, 1.0f, 1.0f},
    .mid_color = {0.6f, 0.2f, 1.0f, 1.0f},
    .outer_color = {0.8f, 0.9f, 1.0f, 1.0f},
    .angle = 0.0f,
    .spread = 0.60f,
    .softness = 2.5f,
    .intensity = 2.2f,
    .speed = 1.2f,
    .offset = 0.2f,
    .direction = -1.0f,
    .grain = 0.02f,
    .pulse = 0.05f,
    .shape = LightLeakStyle::Shape::RadialCorner
};

inline constexpr LightLeakStyle kSunsetDual{
    .id = "tachyon.transition.light_leak_sunset",
    .name = "Sunset Dual",
    .description = "Warm dual-beam sunset leak",
    .inner_color = {1.0f, 0.15f, 0.0f, 1.0f},
    .mid_color = {1.0f, 0.55f, 0.1f, 1.0f},
    .outer_color = {1.0f, 0.95f, 0.6f, 1.0f},
    .angle = 15.0f,
    .spread = 0.45f,
    .softness = 2.2f,
    .intensity = 3.5f,
    .speed = 2.0f,
    .offset = 0.5f,
    .direction = 1.0f,
    .grain = 0.05f,
    .pulse = 0.0f,
    .shape = LightLeakStyle::Shape::DualBeam
};

inline constexpr LightLeakStyle kPaleGhost{
    .id = "tachyon.transition.light_leak_ghost",
    .name = "Pale Ghost",
    .description = "Ethereal pale white leak",
    .inner_color = {0.8f, 0.85f, 1.0f, 1.0f},
    .mid_color = {0.9f, 0.95f, 1.0f, 1.0f},
    .outer_color = {1.0f, 1.0f, 1.0f, 1.0f},
    .angle = -5.0f,
    .spread = 0.65f,
    .softness = 3.0f,
    .intensity = 1.4f,
    .speed = 1.5f,
    .offset = 0.4f,
    .direction = -1.0f,
    .grain = 0.02f,
    .pulse = 0.15f,
    .shape = LightLeakStyle::Shape::Sweep
};

inline constexpr LightLeakStyle kFilmBurn{
    .id = "tachyon.transition.film_burn",
    .name = "Film Burn",
    .description = "Classic fiery film burn",
    .inner_color = {1.0f, 0.2f, 0.0f, 1.0f},
    .mid_color = {1.0f, 0.6f, 0.1f, 1.0f},
    .outer_color = {1.0f, 0.9f, 0.2f, 1.0f},
    .angle = -20.0f,
    .spread = 0.20f,
    .softness = 1.2f,
    .intensity = 4.0f,
    .speed = 2.5f,
    .offset = 0.3f,
    .direction = -1.0f,
    .grain = 0.15f,
    .pulse = 0.0f,
    .shape = LightLeakStyle::Shape::Sweep
};

inline constexpr LightLeakStyle kSoftWarmEdge{
    .id = "tachyon.transition.lightleak.soft_warm_edge",
    .name = "Soft Warm Edge",
    .description = "Gentle warm edge leak",
    .inner_color = {1.0f, 0.3f, 0.1f, 1.0f},
    .mid_color = {1.0f, 0.6f, 0.2f, 1.0f},
    .outer_color = {1.0f, 0.9f, 0.7f, 1.0f},
    .angle = 0.0f,
    .spread = 0.50f,
    .softness = 2.0f,
    .intensity = 1.2f,
    .speed = 1.0f,
    .offset = 0.0f,
    .direction = -1.0f,
    .grain = 0.04f,
    .pulse = 0.0f,
    .shape = LightLeakStyle::Shape::Edge
};

inline constexpr LightLeakStyle kGoldenSweep{
    .id = "tachyon.transition.lightleak.golden_sweep",
    .name = "Golden Sweep",
    .description = "Premium golden sweep",
    .inner_color = {1.0f, 0.7f, 0.0f, 1.0f},
    .mid_color = {1.0f, 0.9f, 0.2f, 1.0f},
    .outer_color = {1.0f, 1.0f, 0.5f, 1.0f},
    .angle = 25.0f,
    .spread = 0.65f,
    .softness = 1.5f,
    .intensity = 4.0f,
    .speed = 2.0f,
    .offset = 0.4f,
    .direction = 1.0f,
    .grain = 0.12f,
    .pulse = 0.0f,
    .shape = LightLeakStyle::Shape::Sweep
};

inline constexpr LightLeakStyle kCreamyWhite{
    .id = "tachyon.transition.lightleak.creamy_white",
    .name = "Creamy White",
    .description = "Soft creamy white leak",
    .inner_color = {1.0f, 0.98f, 0.95f, 1.0f},
    .mid_color = {1.0f, 1.0f, 1.0f, 1.0f},
    .outer_color = {1.0f, 1.0f, 1.0f, 1.0f},
    .angle = 0.0f,
    .spread = 0.30f,
    .softness = 1.8f,
    .intensity = 1.5f,
    .speed = 1.2f,
    .offset = 0.0f,
    .direction = 1.0f,
    .grain = 0.02f,
    .pulse = 0.0f,
    .shape = LightLeakStyle::Shape::HorizontalBand
};

inline constexpr LightLeakStyle kDustyArchive{
    .id = "tachyon.transition.lightleak.dusty_archive",
    .name = "Dusty Archive",
    .description = "Textured vintage leak",
    .inner_color = {0.7f, 0.4f, 0.2f, 1.0f},
    .mid_color = {0.9f, 0.6f, 0.3f, 1.0f},
    .outer_color = {1.0f, 0.9f, 0.7f, 1.0f},
    .angle = -35.0f,
    .spread = 0.18f,
    .softness = 2.0f,
    .intensity = 1.8f,
    .speed = 1.4f,
    .offset = 0.3f,
    .direction = 1.0f,
    .grain = 0.12f,
    .pulse = 0.0f,
    .shape = LightLeakStyle::Shape::WindowBands
};

inline constexpr LightLeakStyle kLensFlarePass{
    .id = "tachyon.transition.lightleak.lens_flare_pass",
    .name = "Lens Flare Pass",
    .description = "Fast blue flare sweep",
    .inner_color = {0.2f, 0.6f, 1.0f, 1.0f},
    .mid_color = {0.6f, 0.8f, 1.0f, 1.0f},
    .outer_color = {1.0f, 1.0f, 1.0f, 1.0f},
    .angle = 15.0f,
    .spread = 0.04f,
    .softness = 1.2f,
    .intensity = 3.0f,
    .speed = 4.0f,
    .offset = 1.8f,
    .direction = 1.0f,
    .grain = 0.05f,
    .pulse = 0.0f,
    .shape = LightLeakStyle::Shape::Sweep
};

inline constexpr LightLeakStyle kAmberSweep{
    .id = "tachyon.transition.lightleak.amber_sweep",
    .name = "Amber Sweep",
    .description = "Rich fiery amber sweep",
    .inner_color = {1.0f, 0.2f, 0.0f, 1.0f},
    .mid_color = {1.0f, 0.4f, 0.0f, 1.0f},
    .outer_color = {1.0f, 0.6f, 0.0f, 1.0f},
    .angle = -15.0f,
    .spread = 0.85f,
    .softness = 0.4f,
    .intensity = 4.0f,
    .speed = 2.2f,
    .offset = 0.3f,
    .direction = 1.0f,
    .grain = 0.10f,
    .pulse = 0.0f,
    .shape = LightLeakStyle::Shape::Sweep
};

inline constexpr LightLeakStyle kNeonPulse{
    .id = "tachyon.transition.lightleak.neon_pulse",
    .name = "Neon Pulse",
    .description = "Futuristic neon pink leak",
    .inner_color = {1.0f, 0.0f, 0.6f, 1.0f},
    .mid_color = {0.4f, 0.0f, 1.0f, 1.0f},
    .outer_color = {1.0f, 0.8f, 1.0f, 1.0f},
    .angle = 30.0f,
    .spread = 0.30f,
    .softness = 1.5f,
    .intensity = 2.5f,
    .speed = 2.0f,
    .offset = 0.5f,
    .direction = 1.0f,
    .grain = 0.10f,
    .pulse = 0.3f,
    .shape = LightLeakStyle::Shape::Sweep
};

inline constexpr LightLeakStyle kPrismShatter{
    .id = "tachyon.transition.lightleak.prism_shatter",
    .name = "Prism Shatter",
    .description = "Rainbow refractive prism",
    .inner_color = {1.0f, 0.2f, 0.2f, 1.0f},
    .mid_color = {0.2f, 1.0f, 0.2f, 1.0f},
    .outer_color = {0.2f, 0.2f, 1.0f, 1.0f},
    .angle = 45.0f,
    .spread = 0.10f,
    .softness = 1.0f,
    .intensity = 2.0f,
    .speed = 2.5f,
    .offset = 0.6f,
    .direction = 1.0f,
    .grain = 0.05f,
    .pulse = 0.0f,
    .shape = LightLeakStyle::Shape::Prism
};

inline constexpr LightLeakStyle kVintageSepia{
    .id = "tachyon.transition.lightleak.vintage_sepia",
    .name = "Vintage Sepia",
    .description = "Warm sepia memory leak",
    .inner_color = {0.4f, 0.2f, 0.1f, 1.0f},
    .mid_color = {0.7f, 0.5f, 0.3f, 1.0f},
    .outer_color = {0.9f, 0.8f, 0.6f, 1.0f},
    .angle = -10.0f,
    .spread = 0.80f,
    .softness = 3.5f,
    .intensity = 1.2f,
    .speed = 1.0f,
    .offset = 0.2f,
    .direction = -1.0f,
    .grain = 0.15f,
    .pulse = 0.0f,
    .shape = LightLeakStyle::Shape::Edge
};

inline constexpr LightLeakStyle kOrganicBlobs{
    .id = "tachyon.transition.lightleak.organic_blobs",
    .name = "Organic Blobs",
    .description = "Fluid Gaussian light blobs",
    .inner_color = {1.0f, 0.2f, 0.0f, 1.0f},
    .mid_color = {1.0f, 0.5f, 0.0f, 1.0f},
    .outer_color = {1.0f, 0.9f, 0.3f, 1.0f},
    .angle = 0.0f,
    .spread = 1.0f,
    .softness = 1.0f,
    .intensity = 4.5f,
    .speed = 1.3f,
    .offset = 0.0f,
    .direction = 1.0f,
    .grain = 0.05f,
    .pulse = 0.1f,
    .shape = LightLeakStyle::Shape::Blobs
};

inline constexpr LightLeakStyle kLavaFlow{
    .id = "tachyon.transition.lightleak.lava_flow",
    .name = "Lava Flow",
    .description = "Intense rising magma blobs",
    .inner_color = {0.90f, 0.02f, 0.0f, 1.0f},
    .mid_color = {1.0f, 0.40f, 0.05f, 1.0f},
    .outer_color = {1.0f, 0.90f, 0.4f, 1.0f},
    .angle = 0.0f,
    .spread = 1.0f,
    .softness = 1.0f,
    .intensity = 5.2f,
    .speed = 1.0f,
    .offset = 0.0f,
    .direction = 1.0f,
    .grain = 0.05f,
    .pulse = 0.15f,
    .shape = LightLeakStyle::Shape::LavaFlow
};

inline constexpr LightLeakStyle kLiquidFission{
    .id = "tachyon.transition.lightleak.liquid_fission",
    .name = "Liquid Fission",
    .description = "Gigantic colliding super-blobs",
    .inner_color = {1.00f, 0.30f, 0.02f, 1.0f},
    .mid_color = {1.0f, 0.70f, 0.05f, 1.0f},
    .outer_color = {1.0f, 1.0f, 0.92f, 1.0f},
    .angle = 0.0f,
    .spread = 1.0f,
    .softness = 1.0f,
    .intensity = 6.5f,
    .speed = 0.9f,
    .offset = 0.0f,
    .direction = 1.0f,
    .grain = 0.08f,
    .pulse = 0.0f,
    .shape = LightLeakStyle::Shape::LiquidFission
};

inline constexpr LightLeakStyle kCosmicSwirl{
    .id = "tachyon.transition.lightleak.cosmic_swirl",
    .name = "Cosmic Swirl",
    .description = "Ethereal spiraling galaxy blobs",
    .inner_color = {0.15f, 0.02f, 0.85f, 1.0f},
    .mid_color = {0.75f, 0.05f, 0.9f, 1.0f},
    .outer_color = {0.5f, 0.85f, 1.0f, 1.0f},
    .angle = 0.0f,
    .spread = 1.0f,
    .softness = 1.0f,
    .intensity = 4.5f,
    .speed = 1.1f,
    .offset = 0.0f,
    .direction = 1.0f,
    .grain = 0.04f,
    .pulse = 0.1f,
    .shape = LightLeakStyle::Shape::CosmicSwirl
};

inline constexpr LightLeakStyle kCinematicAmber{
    .id = "tachyon.transition.lightleak.cinematic_amber",
    .name = "Cinematic Amber",
    .description = "Dynamically shifting warm amber fluid",
    .inner_color = {1.00f, 0.25f, 0.0f, 1.0f},
    .mid_color = {1.0f, 0.45f, 0.02f, 1.0f},
    .outer_color = {1.0f, 0.85f, 0.25f, 1.0f},
    .angle = 0.0f,
    .spread = 1.0f,
    .softness = 1.0f,
    .intensity = 6.0f,
    .speed = 0.8f,
    .offset = 0.0f,
    .direction = 1.0f,
    .grain = 0.04f,
    .pulse = 0.0f,
    .shape = LightLeakStyle::Shape::CinematicAmber
};

inline constexpr LightLeakStyle kProceduralRemotion{
    .id = "tachyon.transition.lightleak.procedural_remotion",
    .name = "Procedural Remotion",
    .description = "Procedural noise-based cinematic leak",
    .inner_color = {1.0f, 0.45f, 0.0f, 1.0f},
    .mid_color = {1.0f, 0.45f, 0.0f, 1.0f},
    .outer_color = {1.0f, 1.0f, 1.0f, 1.0f},
    .angle = 0.0f,
    .spread = 1.0f,
    .softness = 1.0f,
    .intensity = 1.0f,
    .speed = 1.0f,
    .offset = 0.0f,
    .direction = 1.2f, // Note: In ProceduralRemotion, direction functions as the noise generation seed.
    .grain = 0.0f,
    .pulse = 0.0f,
    .shape = LightLeakStyle::Shape::ProceduralRemotion
};

// --- Unified Template Logic & Table Registry ---

template<const LightLeakStyle& Style>
Color transition_light_leak_template(float u, float v, float t, const SurfaceRGBA& in, const SurfaceRGBA* to) {
    return apply_light_leak_style(u, v, t, in, to, Style);
}

struct LightLeakRegistryEntry {
    std::string_view id;
    CpuTransitionFn fn; // Uses the official signature type
};

// The single comprehensive registry powering dynamic resolution
static constexpr std::array<LightLeakRegistryEntry, 21> kLightLeakRegistry = {{
    { ids::transition::light_leak,                  &transition_light_leak_template<kClassicLeak> },
    { ids::transition::light_leak_solar,            &transition_light_leak_template<kSolarFlare> },
    { ids::transition::light_leak_nebula,           &transition_light_leak_template<kBlueNebula> },
    { ids::transition::light_leak_sunset,           &transition_light_leak_template<kSunsetDual> },
    { ids::transition::light_leak_ghost,            &transition_light_leak_template<kPaleGhost> },
    { ids::transition::film_burn,                   &transition_light_leak_template<kFilmBurn> },
    { ids::transition::lightleak_soft_warm_edge,    &transition_light_leak_template<kSoftWarmEdge> },
    { ids::transition::lightleak_golden_sweep,      &transition_light_leak_template<kGoldenSweep> },
    { ids::transition::lightleak_creamy_white,      &transition_light_leak_template<kCreamyWhite> },
    { ids::transition::lightleak_dusty_archive,     &transition_light_leak_template<kDustyArchive> },
    { ids::transition::lightleak_lens_flare_pass,   &transition_light_leak_template<kLensFlarePass> },
    { ids::transition::lightleak_amber_sweep,       &transition_light_leak_template<kAmberSweep> },
    { ids::transition::lightleak_neon_pulse,        &transition_light_leak_template<kNeonPulse> },
    { ids::transition::lightleak_prism_shatter,     &transition_light_leak_template<kPrismShatter> },
    { ids::transition::lightleak_vintage_sepia,     &transition_light_leak_template<kVintageSepia> },
    { ids::transition::lightleak_organic_blobs,     &transition_light_leak_template<kOrganicBlobs> },
    { ids::transition::lightleak_lava_flow,         &transition_light_leak_template<kLavaFlow> },
    { ids::transition::lightleak_liquid_fission,    &transition_light_leak_template<kLiquidFission> },
    { ids::transition::lightleak_cosmic_swirl,      &transition_light_leak_template<kCosmicSwirl> },
    { ids::transition::lightleak_cinematic_amber,   &transition_light_leak_template<kCinematicAmber> },
    { ids::transition::lightleak_procedural_remotion,&transition_light_leak_template<kProceduralRemotion> }
}};

// --- Compile-Time Static Safety Guarantees ---

consteval bool has_registry_duplicates() {
    for (size_t i = 0; i < kLightLeakRegistry.size(); ++i) {
        for (size_t j = i + 1; j < kLightLeakRegistry.size(); ++j) {
            if (kLightLeakRegistry[i].id == kLightLeakRegistry[j].id) return true;
        }
    }
    return false;
}

static_assert(!has_registry_duplicates(), "FATAL: Duplicate transition identifier detected within the Light Leak Registry.");

void resolve_light_leak_implementations(tachyon::TransitionDescriptor& descriptor) {
    // Premium low-overhead cache-friendly iteration loop
    for (const auto& entry : kLightLeakRegistry) {
        if (entry.id == descriptor.id) {
            descriptor.cpu_fn = entry.fn;
            return;
        }
    }
}

} // namespace tachyon::renderer2d
