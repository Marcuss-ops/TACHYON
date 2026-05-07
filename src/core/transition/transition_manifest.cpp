#include "tachyon/core/transition/transition_descriptor.h"
#include "tachyon/core/ids/builtin_ids.h"
#include <vector>
#include <string_view>

namespace tachyon {

namespace {

/**
 * @brief Declarative specification for a built-in transition.
 * This is a lightweight "authoring" form used to define transitions in tables.
 */
struct TransitionBuiltinSpec {
    std::string_view id;
    std::string_view display_name;
    std::string_view description;
    TransitionKind category;
    TransitionRuntimeKind runtime_kind;
    bool supports_cpu{false};
    bool supports_gpu{false};
    CpuTransitionFn cpu_fn{nullptr};
    GlslTransitionFn glsl_fn{nullptr};
};

/**
 * @brief Factory helper to convert a declarative spec to a full TransitionDescriptor.
 */
TransitionDescriptor make_descriptor(const TransitionBuiltinSpec& spec) {
    TransitionDescriptor desc;
    desc.id = std::string(spec.id);
    desc.display_name = std::string(spec.display_name);
    desc.description = std::string(spec.description);
    desc.category = static_cast<TransitionCategory>(spec.category);
    desc.runtime_kind = spec.runtime_kind;
    desc.capabilities.supports_cpu = spec.supports_cpu;
    desc.capabilities.supports_gpu = spec.supports_gpu;
    desc.cpu_fn = spec.cpu_fn;
    desc.glsl_fn = spec.glsl_fn;
    return desc;
}

// --- Pack: Basic Transitions ---
static const TransitionBuiltinSpec kBasicTransitions[] = {
    { ids::transition::crossfade, "Crossfade", "Simple crossfade transition", TransitionKind::Fade, TransitionRuntimeKind::CpuPixel, true, false },
    { ids::transition::slide, "Slide", "Horizontal slide transition", TransitionKind::Slide, TransitionRuntimeKind::CpuPixel, true, false },
    { ids::transition::slide_up, "Slide Up", "Vertical slide transition", TransitionKind::Slide, TransitionRuntimeKind::CpuPixel, true, false },
    { ids::transition::swipe_left, "Swipe Left", "Swipe the source left to reveal the target", TransitionKind::Slide, TransitionRuntimeKind::CpuPixel, true, false },
    { ids::transition::zoom, "Zoom", "Zoom transition", TransitionKind::Zoom, TransitionRuntimeKind::CpuPixel, true, false },
    { ids::transition::flip, "Flip", "Flip transition", TransitionKind::Flip, TransitionRuntimeKind::CpuPixel, true, false },
    { ids::transition::blur, "Blur", "Blur transition", TransitionKind::Fade, TransitionRuntimeKind::CpuPixel, true, false },
    { ids::transition::fade_to_black, "Fade to Black", "Crossfade through black", TransitionKind::Fade, TransitionRuntimeKind::CpuPixel, true, false },
    { ids::transition::wipe_linear, "Linear Wipe", "Simple left-to-right wipe", TransitionKind::Wipe, TransitionRuntimeKind::CpuPixel, true, false },
    { ids::transition::wipe_angular, "Angular Wipe", "Angular wipe around center", TransitionKind::Wipe, TransitionRuntimeKind::CpuPixel, true, false },
    { ids::transition::push_left, "Push Left", "Push image to the left", TransitionKind::Slide, TransitionRuntimeKind::CpuPixel, true, false },
    { ids::transition::slide_easing, "Slide Easing", "Slide with easing", TransitionKind::Slide, TransitionRuntimeKind::CpuPixel, true, false },
    { ids::transition::circle_iris, "Circle Iris", "Circular iris opener", TransitionKind::Wipe, TransitionRuntimeKind::CpuPixel, true, false }
};

// --- Pack: Artistic Transitions ---
static const TransitionBuiltinSpec kArtisticTransitions[] = {
    { ids::transition::glitch_slice, "Glitch Slice", "Digital glitch slice transition", TransitionKind::Custom, TransitionRuntimeKind::CpuPixel, true, false },
    { ids::transition::pixelate, "Pixelate", "Mosaic pixelation transition", TransitionKind::Custom, TransitionRuntimeKind::CpuPixel, true, false },
    { ids::transition::rgb_split, "RGB Split", "Chromatic aberration split", TransitionKind::Custom, TransitionRuntimeKind::CpuPixel, true, false },
    { ids::transition::kaleidoscope, "Kaleidoscope", "Radial mirror kaleidoscope", TransitionKind::Custom, TransitionRuntimeKind::CpuPixel, true, false },
    { ids::transition::ripple, "Ripple", "Water ripple distortion", TransitionKind::Custom, TransitionRuntimeKind::CpuPixel, true, false },
    { ids::transition::spin, "Spin", "Spinning rotation transition", TransitionKind::Custom, TransitionRuntimeKind::CpuPixel, true, false },
    { ids::transition::luma_dissolve, "Luma Dissolve", "Luminance-based dissolve", TransitionKind::Dissolve, TransitionRuntimeKind::CpuPixel, true, false },
    { ids::transition::zoom_blur, "Zoom Blur", "Radial zoom blur expansion", TransitionKind::Zoom, TransitionRuntimeKind::CpuPixel, true, false },
    { ids::transition::zoom_in, "Zoom In", "Fast zoom in transition", TransitionKind::Zoom, TransitionRuntimeKind::CpuPixel, true, false },
    { ids::transition::directional_blur_wipe, "Blur Wipe", "Motion blur directional wipe", TransitionKind::Wipe, TransitionRuntimeKind::CpuPixel, true, false }
};

// --- Pack: Light Leaks ---
static const TransitionBuiltinSpec kLightLeakTransitions[] = {
    { ids::transition::light_leak, "Classic Leak", "Wide amber cinematic leak", TransitionKind::Fade, TransitionRuntimeKind::CpuPixel, true, false },
    { ids::transition::light_leak_solar, "Solar Flare", "Bright golden solar flare", TransitionKind::Fade, TransitionRuntimeKind::CpuPixel, true, false },
    { ids::transition::light_leak_nebula, "Blue Nebula", "Cosmic blue and purple leak", TransitionKind::Fade, TransitionRuntimeKind::CpuPixel, true, false },
    { ids::transition::light_leak_sunset, "Sunset Dual", "Warm dual-beam sunset leak", TransitionKind::Fade, TransitionRuntimeKind::CpuPixel, true, false },
    { ids::transition::light_leak_ghost, "Pale Ghost", "Ethereal pale white leak", TransitionKind::Fade, TransitionRuntimeKind::CpuPixel, true, false },
    { ids::transition::film_burn, "Film Burn", "Classic fiery film burn", TransitionKind::Fade, TransitionRuntimeKind::CpuPixel, true, false },
    { ids::transition::lightleak_soft_warm_edge, "Soft Warm Edge", "Gentle warm edge leak", TransitionKind::Fade, TransitionRuntimeKind::CpuPixel, true, false },
    { ids::transition::lightleak_golden_sweep, "Golden Sweep", "Premium golden sweep", TransitionKind::Fade, TransitionRuntimeKind::CpuPixel, true, false },
    { ids::transition::lightleak_creamy_white, "Creamy White", "Soft creamy white leak", TransitionKind::Fade, TransitionRuntimeKind::CpuPixel, true, false },
    { ids::transition::lightleak_dusty_archive, "Dusty Archive", "Textured vintage leak", TransitionKind::Fade, TransitionRuntimeKind::CpuPixel, true, false },
    { ids::transition::lightleak_lens_flare_pass, "Lens Flare Pass", "Fast blue flare sweep", TransitionKind::Fade, TransitionRuntimeKind::CpuPixel, true, false },
    { ids::transition::lightleak_amber_sweep, "Amber Sweep", "Dynamic amber sweep", TransitionKind::Fade, TransitionRuntimeKind::CpuPixel, true, false },
    { ids::transition::lightleak_neon_pulse, "Neon Pulse", "Futuristic neon pink leak", TransitionKind::Fade, TransitionRuntimeKind::CpuPixel, true, false },
    { ids::transition::lightleak_prism_shatter, "Prism Shatter", "Rainbow refractive prism", TransitionKind::Fade, TransitionRuntimeKind::CpuPixel, true, false },
    { ids::transition::lightleak_vintage_sepia, "Vintage Sepia", "Warm sepia memory leak", TransitionKind::Fade, TransitionRuntimeKind::CpuPixel, true, false }
};

/**
 * @brief Helper to register a table of built-in specs.
 */
template<typename T, size_t N>
void register_table(const T(&table)[N]) {
    for (const auto& spec : table) {
        register_transition_descriptor(make_descriptor(spec));
    }
}

/**
 * @brief Helper to append a table of built-in specs to the manifest vector.
 */
template<typename T, size_t N>
void append_to_manifest(std::vector<TransitionDescriptor>& manifest, const T(&table)[N]) {
    for (const auto& spec : table) {
        manifest.push_back(make_descriptor(spec));
    }
}

} // namespace

const std::vector<TransitionDescriptor>& get_transition_manifest() {
    static std::vector<TransitionDescriptor> manifest;
    if (manifest.empty()) {
        append_to_manifest(manifest, kBasicTransitions);
        append_to_manifest(manifest, kArtisticTransitions);
        append_to_manifest(manifest, kLightLeakTransitions);
    }
    return manifest;
}

void register_builtin_transitions() {
    register_table(kBasicTransitions);
    register_table(kArtisticTransitions);
    register_table(kLightLeakTransitions);
}

} // namespace tachyon
