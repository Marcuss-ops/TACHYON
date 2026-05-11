#include "light_leak_internal.h"
#include "tachyon/renderer2d/effects/core/transitions/light_leak_transitions.h"
#include "tachyon/transition_registry.h"
#include "tachyon/core/transition/transition_descriptor.h"
#include "tachyon/core/ids/builtin_ids.h"
#include <algorithm>

namespace tachyon::renderer2d {

// --- Style Definitions ---

static constexpr LightLeakStyle kClassicLeak{
    "tachyon.transition.light_leak", "Classic Leak", "Wide amber cinematic leak",
    {1.0f, 0.45f, 0.05f, 1.0f}, {1.0f, 0.78f, 0.30f, 1.0f}, {1.0f, 0.98f, 0.92f, 1.0f},
    -22.0f, 0.55f, 2.0f, 1.8f, 2.2f, 0.6f, -1.0f, 0.05f, 0.0f, LightLeakStyle::Shape::Sweep
};

static constexpr LightLeakStyle kSolarFlare{
    "tachyon.transition.light_leak_solar", "Solar Flare", "Bright golden solar flare",
    {1.0f, 0.9f, 0.2f, 1.0f}, {1.0f, 1.0f, 0.6f, 1.0f}, {1.0f, 1.0f, 1.0f, 1.0f},
    45.0f, 0.40f, 1.5f, 3.0f, 1.8f, 0.4f, 1.0f, 0.08f, 0.1f, LightLeakStyle::Shape::RadialCorner
};

static constexpr LightLeakStyle kBlueNebula{
    "tachyon.transition.light_leak_nebula", "Blue Nebula", "Cosmic blue and purple leak",
    {0.0f, 0.3f, 1.0f, 1.0f}, {0.6f, 0.2f, 1.0f, 1.0f}, {0.8f, 0.9f, 1.0f, 1.0f},
    0.0f, 0.60f, 2.5f, 2.2f, 1.2f, 0.2f, -1.0f, 0.02f, 0.05f, LightLeakStyle::Shape::RadialCorner
};

static constexpr LightLeakStyle kSunsetDual{
    "tachyon.transition.light_leak_sunset", "Sunset Dual", "Warm dual-beam sunset leak",
    {1.0f, 0.15f, 0.0f, 1.0f}, {1.0f, 0.55f, 0.1f, 1.0f}, {1.0f, 0.95f, 0.6f, 1.0f},
    15.0f, 0.45f, 2.2f, 3.5f, 2.0f, 0.5f, 1.0f, 0.05f, 0.0f, LightLeakStyle::Shape::DualBeam
};

static constexpr LightLeakStyle kPaleGhost{
    "tachyon.transition.light_leak_ghost", "Pale Ghost", "Ethereal pale white leak",
    {0.8f, 0.85f, 1.0f, 1.0f}, {0.9f, 0.95f, 1.0f, 1.0f}, {1.0f, 1.0f, 1.0f, 1.0f},
    -5.0f, 0.65f, 3.0f, 1.4f, 1.5f, 0.4f, -1.0f, 0.02f, 0.15f, LightLeakStyle::Shape::Sweep
};

static constexpr LightLeakStyle kFilmBurn{
    "tachyon.transition.film_burn", "Film Burn", "Classic fiery film burn",
    {1.0f, 0.2f, 0.0f, 1.0f}, {1.0f, 0.6f, 0.1f, 1.0f}, {1.0f, 0.9f, 0.2f, 1.0f},
    -20.0f, 0.20f, 1.2f, 4.0f, 2.5f, 0.3f, -1.0f, 0.15f, 0.0f, LightLeakStyle::Shape::Sweep
};

static constexpr LightLeakStyle kSoftWarmEdge{
    "tachyon.transition.lightleak.soft_warm_edge", "Soft Warm Edge", "Gentle warm edge leak",
    {1.0f, 0.3f, 0.1f, 1.0f}, {1.0f, 0.6f, 0.2f, 1.0f}, {1.0f, 0.9f, 0.7f, 1.0f},
    0.0f, 0.50f, 2.0f, 1.2f, 1.0f, 0.0f, -1.0f, 0.04f, 0.0f, LightLeakStyle::Shape::Edge
};

static constexpr LightLeakStyle kGoldenSweep{
    "tachyon.transition.lightleak.golden_sweep", "Golden Sweep", "Premium golden sweep",
    {1.0f, 0.7f, 0.0f, 1.0f}, {1.0f, 0.9f, 0.2f, 1.0f}, {1.0f, 1.0f, 0.5f, 1.0f},
    25.0f, 0.65f, 1.5f, 4.0f, 2.0f, 0.4f, 1.0f, 0.12f, 0.0f, LightLeakStyle::Shape::Sweep
};

static constexpr LightLeakStyle kCreamyWhite{
    "tachyon.transition.lightleak.creamy_white", "Creamy White", "Soft creamy white leak",
    {1.0f, 0.98f, 0.95f, 1.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 1.0f, 1.0f},
    0.0f, 0.30f, 1.8f, 1.5f, 1.2f, 0.0f, 1.0f, 0.02f, 0.0f, LightLeakStyle::Shape::HorizontalBand
};

static constexpr LightLeakStyle kDustyArchive{
    "tachyon.transition.lightleak.dusty_archive", "Dusty Archive", "Textured vintage leak",
    {0.7f, 0.4f, 0.2f, 1.0f}, {0.9f, 0.6f, 0.3f, 1.0f}, {1.0f, 0.9f, 0.7f, 1.0f},
    -35.0f, 0.18f, 2.0f, 1.8f, 1.4f, 0.3f, 1.0f, 0.12f, 0.0f, LightLeakStyle::Shape::WindowBands
};

static constexpr LightLeakStyle kLensFlarePass{
    "tachyon.transition.lightleak.lens_flare_pass", "Lens Flare Pass", "Fast blue flare sweep",
    {0.2f, 0.6f, 1.0f, 1.0f}, {0.6f, 0.8f, 1.0f, 1.0f}, {1.0f, 1.0f, 1.0f, 1.0f},
    15.0f, 0.04f, 1.2f, 3.0f, 4.0f, 1.8f, 1.0f, 0.05f, 0.0f, LightLeakStyle::Shape::Sweep
};

static constexpr LightLeakStyle kAmberSweep{
    "tachyon.transition.lightleak.amber_sweep", "Amber Sweep", "Rich fiery amber sweep",
    {1.0f, 0.2f, 0.0f, 1.0f}, {1.0f, 0.4f, 0.0f, 1.0f}, {1.0f, 0.6f, 0.0f, 1.0f},
    -15.0f, 0.85f, 0.4f, 4.0f, 2.2f, 0.3f, 1.0f, 0.10f, 0.0f, LightLeakStyle::Shape::Sweep
};

static constexpr LightLeakStyle kNeonPulse{
    "tachyon.transition.lightleak.neon_pulse", "Neon Pulse", "Futuristic neon pink leak",
    {1.0f, 0.0f, 0.6f, 1.0f}, {0.4f, 0.0f, 1.0f, 1.0f}, {1.0f, 0.8f, 1.0f, 1.0f},
    30.0f, 0.30f, 1.5f, 2.5f, 2.0f, 0.5f, 1.0f, 0.10f, 0.3f, LightLeakStyle::Shape::Sweep
};

static constexpr LightLeakStyle kPrismShatter{
    "tachyon.transition.lightleak.prism_shatter", "Prism Shatter", "Rainbow refractive prism",
    {1.0f, 0.2f, 0.2f, 1.0f}, {0.2f, 1.0f, 0.2f, 1.0f}, {0.2f, 0.2f, 1.0f, 1.0f},
    45.0f, 0.10f, 1.0f, 2.0f, 2.5f, 0.6f, 1.0f, 0.05f, 0.0f, LightLeakStyle::Shape::Prism
};

static constexpr LightLeakStyle kVintageSepia{
    "tachyon.transition.lightleak.vintage_sepia", "Vintage Sepia", "Warm sepia memory leak",
    {0.4f, 0.2f, 0.1f, 1.0f}, {0.7f, 0.5f, 0.3f, 1.0f}, {0.9f, 0.8f, 0.6f, 1.0f},
    -10.0f, 0.80f, 3.5f, 1.2f, 1.0f, 0.2f, -1.0f, 0.15f, 0.0f, LightLeakStyle::Shape::Edge
};

static constexpr LightLeakStyle kOrganicBlobs{
    "tachyon.transition.lightleak.organic_blobs", "Organic Blobs", "Fluid Gaussian light blobs",
    {1.0f, 0.2f, 0.0f, 1.0f}, {1.0f, 0.5f, 0.0f, 1.0f}, {1.0f, 0.9f, 0.3f, 1.0f},
    0.0f, 1.0f, 1.0f, 4.5f, 1.3f, 0.0f, 1.0f, 0.05f, 0.1f, LightLeakStyle::Shape::Blobs
};

static constexpr LightLeakStyle kLavaFlow{
    "tachyon.transition.lightleak.lava_flow", "Lava Flow", "Intense rising magma blobs",
    {0.90f, 0.02f, 0.0f, 1.0f}, {1.0f, 0.40f, 0.05f, 1.0f}, {1.0f, 0.90f, 0.4f, 1.0f},
    0.0f, 1.0f, 1.0f, 5.2f, 1.0f, 0.0f, 1.0f, 0.05f, 0.15f, LightLeakStyle::Shape::LavaFlow
};

static constexpr LightLeakStyle kLiquidFission{
    "tachyon.transition.lightleak.liquid_fission", "Liquid Fission", "Gigantic colliding super-blobs",
    {1.00f, 0.30f, 0.02f, 1.0f}, {1.0f, 0.70f, 0.05f, 1.0f}, {1.0f, 1.0f, 0.92f, 1.0f},
    0.0f, 1.0f, 1.0f, 6.5f, 0.9f, 0.0f, 1.0f, 0.08f, 0.0f, LightLeakStyle::Shape::LiquidFission
};

static constexpr LightLeakStyle kCosmicSwirl{
    "tachyon.transition.lightleak.cosmic_swirl", "Cosmic Swirl", "Ethereal spiraling galaxy blobs",
    {0.15f, 0.02f, 0.85f, 1.0f}, {0.75f, 0.05f, 0.9f, 1.0f}, {0.5f, 0.85f, 1.0f, 1.0f},
    0.0f, 1.0f, 1.0f, 4.5f, 1.1f, 0.0f, 1.0f, 0.04f, 0.1f, LightLeakStyle::Shape::CosmicSwirl
};

static constexpr LightLeakStyle kCinematicAmber{
    "tachyon.transition.lightleak.cinematic_amber", "Cinematic Amber", "Dynamically shifting warm amber fluid",
    {1.00f, 0.25f, 0.0f, 1.0f}, {1.0f, 0.45f, 0.02f, 1.0f}, {1.0f, 0.85f, 0.25f, 1.0f},
    0.0f, 1.0f, 1.0f, 6.0f, 0.8f, 0.0f, 1.0f, 0.04f, 0.0f, LightLeakStyle::Shape::CinematicAmber
};

static constexpr LightLeakStyle kProceduralRemotion{
    "tachyon.transition.lightleak.procedural_remotion", "Procedural Remotion", "Procedural noise-based cinematic leak",
    {1.0f, 0.45f, 0.0f, 1.0f}, {1.0f, 0.45f, 0.0f, 1.0f}, {1.0f, 1.0f, 1.0f, 1.0f},
    0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.2f, 0.0f, 0.0f, LightLeakStyle::Shape::ProceduralRemotion
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
DEFINE_LEAK_WRAPPER(lightleak_lava_flow, kLavaFlow)
DEFINE_LEAK_WRAPPER(lightleak_liquid_fission, kLiquidFission)
DEFINE_LEAK_WRAPPER(lightleak_cosmic_swirl, kCosmicSwirl)
DEFINE_LEAK_WRAPPER(lightleak_cinematic_amber, kCinematicAmber)
DEFINE_LEAK_WRAPPER(lightleak_procedural_remotion, kProceduralRemotion)

void resolve_light_leak_implementations(tachyon::TransitionDescriptor& d) {
    using namespace tachyon;

    if (d.id == ids::transition::light_leak) d.cpu_fn = transition_light_leak;
    else if (d.id == ids::transition::light_leak_solar) d.cpu_fn = transition_light_leak_solar;
    else if (d.id == ids::transition::light_leak_nebula) d.cpu_fn = transition_light_leak_nebula;
    else if (d.id == ids::transition::light_leak_sunset) d.cpu_fn = transition_light_leak_sunset;
    else if (d.id == ids::transition::light_leak_ghost) d.cpu_fn = transition_light_leak_ghost;
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
    else if (d.id == ids::transition::lightleak_lava_flow) d.cpu_fn = transition_lightleak_lava_flow;
    else if (d.id == ids::transition::lightleak_liquid_fission) d.cpu_fn = transition_lightleak_liquid_fission;
    else if (d.id == ids::transition::lightleak_cosmic_swirl) d.cpu_fn = transition_lightleak_cosmic_swirl;
    else if (d.id == ids::transition::lightleak_cinematic_amber) d.cpu_fn = transition_lightleak_cinematic_amber;
    else if (d.id == ids::transition::lightleak_procedural_remotion) d.cpu_fn = transition_lightleak_procedural_remotion;
}

} // namespace tachyon::renderer2d
