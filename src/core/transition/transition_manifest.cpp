#include "tachyon/core/transition/transition_descriptor.h"
#include "tachyon/core/ids/builtin_ids.h"
#include <vector>

namespace tachyon {

namespace {

TransitionDescriptor make_desc(
    std::string id,
    std::string display_name,
    std::string description,
    TransitionKind kind,
    TransitionRuntimeKind runtime_kind,
    bool supports_cpu,
    bool supports_gpu)
{
    TransitionDescriptor desc;
    desc.id = std::move(id);
    desc.display_name = std::move(display_name);
    desc.description = std::move(description);
    desc.category = static_cast<TransitionCategory>(kind);
    desc.runtime_kind = runtime_kind;
    desc.capabilities.supports_cpu = supports_cpu;
    desc.capabilities.supports_gpu = supports_gpu;
    return desc;
}

} // namespace

const std::vector<TransitionDescriptor>& get_transition_manifest() {
    static const std::vector<TransitionDescriptor> manifest = {
        // --- Basic Transitions ---
        make_desc(std::string(ids::transition::crossfade), "Crossfade", "Simple crossfade transition", TransitionKind::Fade, TransitionRuntimeKind::CpuPixel, true, false),
        make_desc(std::string(ids::transition::slide), "Slide", "Horizontal slide transition", TransitionKind::Slide, TransitionRuntimeKind::CpuPixel, true, false),
        make_desc(std::string(ids::transition::slide_up), "Slide Up", "Vertical slide transition", TransitionKind::Slide, TransitionRuntimeKind::CpuPixel, true, false),
        make_desc(std::string(ids::transition::swipe_left), "Swipe Left", "Swipe the source left to reveal the target", TransitionKind::Slide, TransitionRuntimeKind::CpuPixel, true, false),
        make_desc(std::string(ids::transition::zoom), "Zoom", "Zoom transition", TransitionKind::Zoom, TransitionRuntimeKind::CpuPixel, true, false),
        make_desc(std::string(ids::transition::flip), "Flip", "Flip transition", TransitionKind::Flip, TransitionRuntimeKind::CpuPixel, true, false),
        make_desc(std::string(ids::transition::blur), "Blur", "Blur transition", TransitionKind::Fade, TransitionRuntimeKind::CpuPixel, true, false),
        make_desc(std::string(ids::transition::fade_to_black), "Fade to Black", "Crossfade through black", TransitionKind::Fade, TransitionRuntimeKind::CpuPixel, true, false),
        make_desc(std::string(ids::transition::wipe_linear), "Linear Wipe", "Simple left-to-right wipe", TransitionKind::Wipe, TransitionRuntimeKind::CpuPixel, true, false),
        make_desc(std::string(ids::transition::wipe_angular), "Angular Wipe", "Angular wipe around center", TransitionKind::Wipe, TransitionRuntimeKind::CpuPixel, true, false),
        make_desc(std::string(ids::transition::push_left), "Push Left", "Push image to the left", TransitionKind::Slide, TransitionRuntimeKind::CpuPixel, true, false),
        make_desc(std::string(ids::transition::slide_easing), "Slide Easing", "Slide with easing", TransitionKind::Slide, TransitionRuntimeKind::CpuPixel, true, false),
        make_desc(std::string(ids::transition::circle_iris), "Circle Iris", "Circular iris opener", TransitionKind::Wipe, TransitionRuntimeKind::CpuPixel, true, false),

        // --- Artistic Transitions ---
        make_desc(std::string(ids::transition::glitch_slice), "Glitch Slice", "Digital glitch slice transition", TransitionKind::Custom, TransitionRuntimeKind::CpuPixel, true, false),
        make_desc(std::string(ids::transition::pixelate), "Pixelate", "Mosaic pixelation transition", TransitionKind::Custom, TransitionRuntimeKind::CpuPixel, true, false),
        make_desc(std::string(ids::transition::rgb_split), "RGB Split", "Chromatic aberration split", TransitionKind::Custom, TransitionRuntimeKind::CpuPixel, true, false),
        make_desc(std::string(ids::transition::kaleidoscope), "Kaleidoscope", "Radial mirror kaleidoscope", TransitionKind::Custom, TransitionRuntimeKind::CpuPixel, true, false),
        make_desc(std::string(ids::transition::ripple), "Ripple", "Water ripple distortion", TransitionKind::Custom, TransitionRuntimeKind::CpuPixel, true, false),
        make_desc(std::string(ids::transition::spin), "Spin", "Spinning rotation transition", TransitionKind::Custom, TransitionRuntimeKind::CpuPixel, true, false),
        make_desc(std::string(ids::transition::luma_dissolve), "Luma Dissolve", "Luminance-based dissolve", TransitionKind::Dissolve, TransitionRuntimeKind::CpuPixel, true, false),
        make_desc(std::string(ids::transition::zoom_blur), "Zoom Blur", "Radial zoom blur expansion", TransitionKind::Zoom, TransitionRuntimeKind::CpuPixel, true, false),
        make_desc(std::string(ids::transition::zoom_in), "Zoom In", "Fast zoom in transition", TransitionKind::Zoom, TransitionRuntimeKind::CpuPixel, true, false),
        make_desc(std::string(ids::transition::directional_blur_wipe), "Blur Wipe", "Motion blur directional wipe", TransitionKind::Wipe, TransitionRuntimeKind::CpuPixel, true, false),

        // --- Light Leaks Pack ---
        make_desc(std::string(ids::transition::light_leak), "Classic Leak", "Wide amber cinematic leak", TransitionKind::Fade, TransitionRuntimeKind::CpuPixel, true, false),
        make_desc(std::string(ids::transition::light_leak_solar), "Solar Flare", "Bright golden solar flare", TransitionKind::Fade, TransitionRuntimeKind::CpuPixel, true, false),
        make_desc(std::string(ids::transition::light_leak_nebula), "Blue Nebula", "Cosmic blue and purple leak", TransitionKind::Fade, TransitionRuntimeKind::CpuPixel, true, false),
        make_desc(std::string(ids::transition::light_leak_sunset), "Sunset Dual", "Warm dual-beam sunset leak", TransitionKind::Fade, TransitionRuntimeKind::CpuPixel, true, false),
        make_desc(std::string(ids::transition::light_leak_ghost), "Pale Ghost", "Ethereal pale white leak", TransitionKind::Fade, TransitionRuntimeKind::CpuPixel, true, false),
        make_desc(std::string(ids::transition::film_burn), "Film Burn", "Classic fiery film burn", TransitionKind::Fade, TransitionRuntimeKind::CpuPixel, true, false),

        // --- Premium Light Leaks ---
        make_desc(std::string(ids::transition::lightleak_soft_warm_edge), "Soft Warm Edge", "Gentle warm edge leak", TransitionKind::Fade, TransitionRuntimeKind::CpuPixel, true, false),
        make_desc(std::string(ids::transition::lightleak_golden_sweep), "Golden Sweep", "Premium golden sweep", TransitionKind::Fade, TransitionRuntimeKind::CpuPixel, true, false),
        make_desc(std::string(ids::transition::lightleak_creamy_white), "Creamy White", "Soft creamy white leak", TransitionKind::Fade, TransitionRuntimeKind::CpuPixel, true, false),
        make_desc(std::string(ids::transition::lightleak_dusty_archive), "Dusty Archive", "Textured vintage leak", TransitionKind::Fade, TransitionRuntimeKind::CpuPixel, true, false),
        make_desc(std::string(ids::transition::lightleak_lens_flare_pass), "Lens Flare Pass", "Fast blue flare sweep", TransitionKind::Fade, TransitionRuntimeKind::CpuPixel, true, false),
        make_desc(std::string(ids::transition::lightleak_amber_sweep), "Amber Sweep", "Dynamic amber sweep", TransitionKind::Fade, TransitionRuntimeKind::CpuPixel, true, false),
        make_desc(std::string(ids::transition::lightleak_neon_pulse), "Neon Pulse", "Futuristic neon pink leak", TransitionKind::Fade, TransitionRuntimeKind::CpuPixel, true, false),
        make_desc(std::string(ids::transition::lightleak_prism_shatter), "Prism Shatter", "Rainbow refractive prism", TransitionKind::Fade, TransitionRuntimeKind::CpuPixel, true, false),
        make_desc(std::string(ids::transition::lightleak_vintage_sepia), "Vintage Sepia", "Warm sepia memory leak", TransitionKind::Fade, TransitionRuntimeKind::CpuPixel, true, false)
    };
    return manifest;
}

void register_builtin_transitions() {
    const auto& manifest = get_transition_manifest();
    for (const auto& desc : manifest) {
        register_transition_descriptor(desc);
    }
}

} // namespace tachyon
