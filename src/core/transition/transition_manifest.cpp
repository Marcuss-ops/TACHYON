#include "tachyon/core/transition/transition_descriptor.h"
#include "tachyon/core/ids/builtin_ids.h"
#include <vector>

namespace tachyon {

namespace {

TransitionDescriptor make_desc(
    std::string id,
    std::string name,
    std::string description,
    TransitionKind kind,
    TransitionBackend backend,
    std::string renderer_effect_id,
    std::vector<std::string> tags,
    std::string category,
    double duration) 
{
    TransitionDescriptor d;
    d.id = std::move(id);
    d.display_name = std::move(name);
    d.description = std::move(description);
    d.kind = kind;
    d.backend = backend;
    d.renderer_effect_id = std::move(renderer_effect_id);
    d.tags = std::move(tags);
    d.category = std::move(category);
    d.default_duration = duration;
    d.supports_cpu = (backend != TransitionBackend::Glsl);
    d.supports_gpu = (backend == TransitionBackend::Glsl);
    return d;
}

} // namespace

void register_builtin_transitions() {
    const std::vector<TransitionDescriptor> manifest = {
        // --- Basic Transitions ---
        make_desc(std::string(ids::transition::crossfade), "Crossfade", "Simple crossfade transition", TransitionKind::Fade, TransitionBackend::CpuPixel, "transition_crossfade", {"basic"}, "basic", 0.4),
        make_desc(std::string(ids::transition::slide), "Slide", "Horizontal slide transition", TransitionKind::Slide, TransitionBackend::CpuPixel, "transition_slide", {"basic"}, "basic", 0.5),
        make_desc(std::string(ids::transition::slide_up), "Slide Up", "Vertical slide transition", TransitionKind::Slide, TransitionBackend::CpuPixel, "transition_slide_up", {"basic"}, "basic", 0.5),
        make_desc(std::string(ids::transition::swipe_left), "Swipe Left", "Swipe the source left to reveal the target", TransitionKind::Slide, TransitionBackend::CpuPixel, "transition_swipe_left", {"basic"}, "basic", 0.5),
        make_desc(std::string(ids::transition::zoom), "Zoom", "Zoom transition", TransitionKind::Zoom, TransitionBackend::CpuPixel, "transition_zoom", {"basic"}, "basic", 0.6),
        make_desc(std::string(ids::transition::flip), "Flip", "Flip transition", TransitionKind::Flip, TransitionBackend::CpuPixel, "transition_flip", {"basic"}, "basic", 0.6),
        make_desc(std::string(ids::transition::blur), "Blur", "Blur transition", TransitionKind::Fade, TransitionBackend::CpuPixel, "transition_blur", {"basic"}, "basic", 0.5),
        make_desc(std::string(ids::transition::fade_to_black), "Fade to Black", "Crossfade through black", TransitionKind::Fade, TransitionBackend::CpuPixel, "transition_fade_to_black", {"basic"}, "basic", 0.8),
        make_desc(std::string(ids::transition::wipe_linear), "Linear Wipe", "Simple left-to-right wipe", TransitionKind::Wipe, TransitionBackend::CpuPixel, "transition_wipe_linear", {"basic"}, "basic", 0.5),
        make_desc(std::string(ids::transition::wipe_angular), "Angular Wipe", "Angular wipe around center", TransitionKind::Wipe, TransitionBackend::CpuPixel, "transition_wipe_angular", {"basic"}, "basic", 0.7),
        make_desc(std::string(ids::transition::push_left), "Push Left", "Push image to the left", TransitionKind::Slide, TransitionBackend::CpuPixel, "transition_push_left", {"basic"}, "basic", 0.5),
        make_desc(std::string(ids::transition::slide_easing), "Slide Easing", "Slide with easing", TransitionKind::Slide, TransitionBackend::CpuPixel, "transition_slide_easing", {"basic"}, "basic", 0.6),
        make_desc(std::string(ids::transition::circle_iris), "Circle Iris", "Circular iris opener", TransitionKind::Wipe, TransitionBackend::CpuPixel, "transition_circle_iris", {"basic"}, "basic", 0.6),

        // --- Artistic Transitions ---
        make_desc(std::string(ids::transition::glitch_slice), "Glitch Slice", "Digital glitch slice transition", TransitionKind::Custom, TransitionBackend::CpuPixel, "transition_glitch_slice", {"artistic", "glitch"}, "artistic", 0.3),
        make_desc(std::string(ids::transition::pixelate), "Pixelate", "Mosaic pixelation transition", TransitionKind::Custom, TransitionBackend::CpuPixel, "transition_pixelate", {"artistic", "mosaic"}, "artistic", 0.5),
        make_desc(std::string(ids::transition::rgb_split), "RGB Split", "Chromatic aberration split", TransitionKind::Custom, TransitionBackend::CpuPixel, "transition_rgb_split", {"artistic", "chromatic"}, "artistic", 0.4),
        make_desc(std::string(ids::transition::kaleidoscope), "Kaleidoscope", "Radial mirror kaleidoscope", TransitionKind::Custom, TransitionBackend::CpuPixel, "transition_kaleidoscope", {"artistic", "mirror"}, "artistic", 0.7),
        make_desc(std::string(ids::transition::ripple), "Ripple", "Water ripple distortion", TransitionKind::Custom, TransitionBackend::CpuPixel, "transition_ripple", {"artistic", "distortion"}, "artistic", 0.6),
        make_desc(std::string(ids::transition::spin), "Spin", "Spinning rotation transition", TransitionKind::Custom, TransitionBackend::CpuPixel, "transition_spin", {"artistic", "rotation"}, "artistic", 0.5),
        make_desc(std::string(ids::transition::luma_dissolve), "Luma Dissolve", "Luminance-based dissolve", TransitionKind::Dissolve, TransitionBackend::CpuPixel, "transition_luma_dissolve", {"artistic", "luma"}, "artistic", 0.8),
        make_desc(std::string(ids::transition::zoom_blur), "Zoom Blur", "Radial zoom blur expansion", TransitionKind::Zoom, TransitionBackend::CpuPixel, "transition_zoom_blur", {"artistic", "zoom"}, "artistic", 0.4),
        make_desc(std::string(ids::transition::zoom_in), "Zoom In", "Fast zoom in transition", TransitionKind::Zoom, TransitionBackend::CpuPixel, "transition_zoom_in", {"artistic", "fast"}, "artistic", 0.3),
        make_desc(std::string(ids::transition::directional_blur_wipe), "Blur Wipe", "Motion blur directional wipe", TransitionKind::Wipe, TransitionBackend::CpuPixel, "transition_directional_blur_wipe", {"artistic", "blur"}, "artistic", 0.5),

        // --- Light Leaks Pack ---
        make_desc(std::string(ids::transition::light_leak), "Classic Leak", "Wide amber cinematic leak", TransitionKind::Fade, TransitionBackend::CpuPixel, "transition_light_leak", {"premium", "cinematic", "amber"}, "lightleak", 0.6),
        make_desc(std::string(ids::transition::light_leak_solar), "Solar Flare", "Bright golden solar flare", TransitionKind::Fade, TransitionBackend::CpuPixel, "transition_light_leak_solar", {"premium", "solar", "gold"}, "lightleak", 0.4),
        make_desc(std::string(ids::transition::light_leak_nebula), "Blue Nebula", "Cosmic blue and purple leak", TransitionKind::Fade, TransitionBackend::CpuPixel, "transition_light_leak_nebula", {"premium", "cosmic", "blue"}, "lightleak", 0.5),
        make_desc(std::string(ids::transition::light_leak_sunset), "Sunset Dual", "Warm dual-beam sunset leak", TransitionKind::Fade, TransitionBackend::CpuPixel, "transition_light_leak_sunset", {"premium", "warm", "dual"}, "lightleak", 0.5),
        make_desc(std::string(ids::transition::light_leak_ghost), "Pale Ghost", "Ethereal pale white leak", TransitionKind::Fade, TransitionBackend::CpuPixel, "transition_light_leak_ghost", {"premium", "pale", "ghost"}, "lightleak", 0.4),
        make_desc(std::string(ids::transition::film_burn), "Film Burn", "Classic fiery film burn", TransitionKind::Fade, TransitionBackend::CpuPixel, "transition_film_burn", {"premium", "vintage", "burn"}, "lightleak", 0.3),
        
        // --- Premium Light Leaks ---
        make_desc(std::string(ids::transition::lightleak_soft_warm_edge), "Soft Warm Edge", "Gentle warm edge leak", TransitionKind::Fade, TransitionBackend::CpuPixel, "transition_lightleak_soft_warm_edge", {"premium", "soft", "warm"}, "lightleak", 0.5),
        make_desc(std::string(ids::transition::lightleak_golden_sweep), "Golden Sweep", "Premium golden sweep", TransitionKind::Fade, TransitionBackend::CpuPixel, "transition_lightleak_golden_sweep", {"premium", "gold", "sweep"}, "lightleak", 0.6),
        make_desc(std::string(ids::transition::lightleak_creamy_white), "Creamy White", "Soft creamy white leak", TransitionKind::Fade, TransitionBackend::CpuPixel, "transition_lightleak_creamy_white", {"premium", "creamy", "white"}, "lightleak", 0.5),
        make_desc(std::string(ids::transition::lightleak_dusty_archive), "Dusty Archive", "Textured vintage leak", TransitionKind::Fade, TransitionBackend::CpuPixel, "transition_lightleak_dusty_archive", {"premium", "vintage", "textured"}, "lightleak", 0.7),
        make_desc(std::string(ids::transition::lightleak_lens_flare_pass), "Lens Flare Pass", "Fast blue flare sweep", TransitionKind::Fade, TransitionBackend::CpuPixel, "transition_lightleak_lens_flare_pass", {"premium", "flare", "fast"}, "lightleak", 0.3),
        make_desc(std::string(ids::transition::lightleak_amber_sweep), "Amber Sweep", "Dynamic amber sweep", TransitionKind::Fade, TransitionBackend::CpuPixel, "transition_lightleak_amber_sweep", {"premium", "amber", "sweep"}, "lightleak", 0.6),
        make_desc(std::string(ids::transition::lightleak_neon_pulse), "Neon Pulse", "Futuristic neon pink leak", TransitionKind::Fade, TransitionBackend::CpuPixel, "transition_lightleak_neon_pulse", {"premium", "neon", "pink"}, "lightleak", 0.5),
        make_desc(std::string(ids::transition::lightleak_prism_shatter), "Prism Shatter", "Rainbow refractive prism", TransitionKind::Fade, TransitionBackend::CpuPixel, "transition_lightleak_prism_shatter", {"premium", "prism", "refractive"}, "lightleak", 0.5),
        make_desc(std::string(ids::transition::lightleak_vintage_sepia), "Vintage Sepia", "Warm sepia memory leak", TransitionKind::Fade, TransitionBackend::CpuPixel, "transition_lightleak_vintage_sepia", {"premium", "vintage", "sepia"}, "lightleak", 0.6)
    };
    
    for (const auto& desc : manifest) {
        register_transition_descriptor(desc);
    }
}

} // namespace tachyon
