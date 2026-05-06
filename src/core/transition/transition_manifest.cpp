#include "tachyon/core/transition/transition_descriptor.h"
#include "tachyon/core/ids/builtin_ids.h"
#include <vector>

namespace tachyon {

void register_builtin_transitions() {
    const std::vector<TransitionDescriptor> manifest = {
        // --- Basic Transitions ---
        {
            std::string(ids::transition::crossfade), "Crossfade", "Simple crossfade transition",
            TransitionKind::Fade, TransitionBackend::CpuPixel, "transition_crossfade", nullptr,
            {"basic"}, "basic", 0.4, "", true
        },
        {
            std::string(ids::transition::slide), "Slide", "Horizontal slide transition",
            TransitionKind::Slide, TransitionBackend::CpuPixel, "transition_slide", nullptr,
            {"basic"}, "basic", 0.5, "", true
        },
        {
            std::string(ids::transition::slide_up), "Slide Up", "Vertical slide transition",
            TransitionKind::Slide, TransitionBackend::CpuPixel, "transition_slide_up", nullptr,
            {"basic"}, "basic", 0.5, "", true
        },
        {
            std::string(ids::transition::swipe_left), "Swipe Left", "Swipe the source left to reveal the target",
            TransitionKind::Slide, TransitionBackend::CpuPixel, "transition_swipe_left", nullptr,
            {"basic"}, "basic", 0.5, "", true
        },
        {
            std::string(ids::transition::zoom), "Zoom", "Zoom transition",
            TransitionKind::Zoom, TransitionBackend::CpuPixel, "transition_zoom", nullptr,
            {"basic"}, "basic", 0.6, "", true
        },
        {
            std::string(ids::transition::flip), "Flip", "Flip transition",
            TransitionKind::Flip, TransitionBackend::CpuPixel, "transition_flip", nullptr,
            {"basic"}, "basic", 0.6, "", true
        },
        {
            std::string(ids::transition::blur), "Blur", "Blur transition",
            TransitionKind::Fade, TransitionBackend::CpuPixel, "transition_blur", nullptr,
            {"basic"}, "basic", 0.5, "", true
        },
        {
            std::string(ids::transition::fade_to_black), "Fade to Black", "Crossfade through black",
            TransitionKind::Fade, TransitionBackend::CpuPixel, "transition_fade_to_black", nullptr,
            {"basic"}, "basic", 0.8, "", true
        },
        {
            std::string(ids::transition::wipe_linear), "Linear Wipe", "Simple left-to-right wipe",
            TransitionKind::Wipe, TransitionBackend::CpuPixel, "transition_wipe_linear", nullptr,
            {"basic"}, "basic", 0.5, "", true
        },
        {
            std::string(ids::transition::wipe_angular), "Angular Wipe", "Angular wipe around center",
            TransitionKind::Wipe, TransitionBackend::CpuPixel, "transition_wipe_angular", nullptr,
            {"basic"}, "basic", 0.7, "", true
        },
        {
            std::string(ids::transition::push_left), "Push Left", "Push image to the left",
            TransitionKind::Slide, TransitionBackend::CpuPixel, "transition_push_left", nullptr,
            {"basic"}, "basic", 0.5, "", true
        },
        {
            std::string(ids::transition::slide_easing), "Slide Easing", "Slide with easing",
            TransitionKind::Slide, TransitionBackend::CpuPixel, "transition_slide_easing", nullptr,
            {"basic"}, "basic", 0.6, "", true
        },
        {
            std::string(ids::transition::circle_iris), "Circle Iris", "Circular iris opener",
            TransitionKind::Wipe, TransitionBackend::CpuPixel, "transition_circle_iris", nullptr,
            {"basic"}, "basic", 0.6, "", true
        },

        // --- Artistic Transitions ---
        {
            std::string(ids::transition::glitch_slice), "Glitch Slice", "Digital glitch slice transition",
            TransitionKind::Custom, TransitionBackend::CpuPixel, "transition_glitch_slice", nullptr,
            {"artistic", "glitch"}, "artistic", 0.3, "", true
        },
        {
            std::string(ids::transition::pixelate), "Pixelate", "Mosaic pixelation transition",
            TransitionKind::Custom, TransitionBackend::CpuPixel, "transition_pixelate", nullptr,
            {"artistic", "mosaic"}, "artistic", 0.5, "", true
        },
        {
            std::string(ids::transition::rgb_split), "RGB Split", "Chromatic aberration split",
            TransitionKind::Custom, TransitionBackend::CpuPixel, "transition_rgb_split", nullptr,
            {"artistic", "chromatic"}, "artistic", 0.4, "", true
        },
        {
            std::string(ids::transition::kaleidoscope), "Kaleidoscope", "Radial mirror kaleidoscope",
            TransitionKind::Custom, TransitionBackend::CpuPixel, "transition_kaleidoscope", nullptr,
            {"artistic", "mirror"}, "artistic", 0.7, "", true
        },
        {
            std::string(ids::transition::ripple), "Ripple", "Water ripple distortion",
            TransitionKind::Custom, TransitionBackend::CpuPixel, "transition_ripple", nullptr,
            {"artistic", "distortion"}, "artistic", 0.6, "", true
        },
        {
            std::string(ids::transition::spin), "Spin", "Spinning rotation transition",
            TransitionKind::Custom, TransitionBackend::CpuPixel, "transition_spin", nullptr,
            {"artistic", "rotation"}, "artistic", 0.5, "", true
        },
        {
            std::string(ids::transition::luma_dissolve), "Luma Dissolve", "Luminance-based dissolve",
            TransitionKind::Dissolve, TransitionBackend::CpuPixel, "transition_luma_dissolve", nullptr,
            {"artistic", "luma"}, "artistic", 0.8, "", true
        },
        {
            std::string(ids::transition::zoom_blur), "Zoom Blur", "Radial zoom blur expansion",
            TransitionKind::Zoom, TransitionBackend::CpuPixel, "transition_zoom_blur", nullptr,
            {"artistic", "zoom"}, "artistic", 0.4, "", true
        },
        {
            std::string(ids::transition::zoom_in), "Zoom In", "Fast zoom in transition",
            TransitionKind::Zoom, TransitionBackend::CpuPixel, "transition_zoom_in", nullptr,
            {"artistic", "fast"}, "artistic", 0.3, "", true
        },
        {
            std::string(ids::transition::directional_blur_wipe), "Blur Wipe", "Motion blur directional wipe",
            TransitionKind::Wipe, TransitionBackend::CpuPixel, "transition_directional_blur_wipe", nullptr,
            {"artistic", "blur"}, "artistic", 0.5, "", true
        },

        // --- Light Leaks Pack ---
        {
            std::string(ids::transition::light_leak), "Classic Leak", "Wide amber cinematic leak",
            TransitionKind::Fade, TransitionBackend::CpuPixel, "transition_light_leak", nullptr,
            {"premium", "cinematic", "amber"}, "lightleak", 0.6, "", true
        },
        {
            std::string(ids::transition::light_leak_solar), "Solar Flare", "Bright golden solar flare",
            TransitionKind::Fade, TransitionBackend::CpuPixel, "transition_light_leak_solar", nullptr,
            {"premium", "solar", "gold"}, "lightleak", 0.4, "", true
        },
        {
            std::string(ids::transition::light_leak_nebula), "Blue Nebula", "Cosmic blue and purple leak",
            TransitionKind::Fade, TransitionBackend::CpuPixel, "transition_light_leak_nebula", nullptr,
            {"premium", "cosmic", "blue"}, "lightleak", 0.5, "", true
        },
        {
            std::string(ids::transition::light_leak_sunset), "Sunset Dual", "Warm dual-beam sunset leak",
            TransitionKind::Fade, TransitionBackend::CpuPixel, "transition_light_leak_sunset", nullptr,
            {"premium", "warm", "dual"}, "lightleak", 0.5, "", true
        },
        {
            std::string(ids::transition::light_leak_ghost), "Pale Ghost", "Ethereal pale white leak",
            TransitionKind::Fade, TransitionBackend::CpuPixel, "transition_light_leak_ghost", nullptr,
            {"premium", "pale", "ghost"}, "lightleak", 0.4, "", true
        },
        {
            std::string(ids::transition::film_burn), "Film Burn", "Classic fiery film burn",
            TransitionKind::Fade, TransitionBackend::CpuPixel, "transition_film_burn", nullptr,
            {"premium", "vintage", "burn"}, "lightleak", 0.3, "", true
        },
        
        // --- Premium Light Leaks ---
        {
            std::string(ids::transition::lightleak_soft_warm_edge), "Soft Warm Edge", "Gentle warm edge leak",
            TransitionKind::Fade, TransitionBackend::CpuPixel, "transition_lightleak_soft_warm_edge", nullptr,
            {"premium", "soft", "warm"}, "lightleak", 0.5, "", true
        },
        {
            std::string(ids::transition::lightleak_golden_sweep), "Golden Sweep", "Premium golden sweep",
            TransitionKind::Fade, TransitionBackend::CpuPixel, "transition_lightleak_golden_sweep", nullptr,
            {"premium", "gold", "sweep"}, "lightleak", 0.6, "", true
        },
        {
            std::string(ids::transition::lightleak_creamy_white), "Creamy White", "Soft creamy white leak",
            TransitionKind::Fade, TransitionBackend::CpuPixel, "transition_lightleak_creamy_white", nullptr,
            {"premium", "creamy", "white"}, "lightleak", 0.5, "", true
        },
        {
            std::string(ids::transition::lightleak_dusty_archive), "Dusty Archive", "Textured vintage leak",
            TransitionKind::Fade, TransitionBackend::CpuPixel, "transition_lightleak_dusty_archive", nullptr,
            {"premium", "vintage", "textured"}, "lightleak", 0.7, "", true
        },
        {
            std::string(ids::transition::lightleak_lens_flare_pass), "Lens Flare Pass", "Fast blue flare sweep",
            TransitionKind::Fade, TransitionBackend::CpuPixel, "transition_lightleak_lens_flare_pass", nullptr,
            {"premium", "flare", "fast"}, "lightleak", 0.3, "", true
        },
        {
            std::string(ids::transition::lightleak_amber_sweep), "Amber Sweep", "Dynamic amber sweep",
            TransitionKind::Fade, TransitionBackend::CpuPixel, "transition_lightleak_amber_sweep", nullptr,
            {"premium", "amber", "sweep"}, "lightleak", 0.6, "", true
        },
        {
            std::string(ids::transition::lightleak_neon_pulse), "Neon Pulse", "Futuristic neon pink leak",
            TransitionKind::Fade, TransitionBackend::CpuPixel, "transition_lightleak_neon_pulse", nullptr,
            {"premium", "neon", "pink"}, "lightleak", 0.5, "", true
        },
        {
            std::string(ids::transition::lightleak_prism_shatter), "Prism Shatter", "Rainbow refractive prism",
            TransitionKind::Fade, TransitionBackend::CpuPixel, "transition_lightleak_prism_shatter", nullptr,
            {"premium", "prism", "refractive"}, "lightleak", 0.5, "", true
        },
        {
            std::string(ids::transition::lightleak_vintage_sepia), "Vintage Sepia", "Warm sepia memory leak",
            TransitionKind::Fade, TransitionBackend::CpuPixel, "transition_lightleak_vintage_sepia", nullptr,
            {"premium", "vintage", "sepia"}, "lightleak", 0.6, "", true
        }
    };
    
    for (const auto& desc : manifest) {
        register_transition_descriptor(desc);
    }
}

} // namespace tachyon
