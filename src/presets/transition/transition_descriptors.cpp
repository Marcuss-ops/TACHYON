#include "tachyon/transition_catalog.h"
#include "tachyon/core/ids/builtin_ids.h"

namespace tachyon {

std::vector<CatalogTransitionDescriptor> get_builtin_transition_descriptors() {
    return {
        {
            std::string(ids::transition::fade),
            std::string(ids::transition::fade),
            "Fade",
            "Crossfade between layers",
            TransitionKind::Fade,
            TransitionStatus::Stable,
            {"fade", "crossfade", "dissolve"}
        },
        {
            std::string(ids::transition::slide_left),
            std::string(ids::transition::slide_left),
            "Slide Left",
            "Slide layer from left",
            TransitionKind::Slide,
            TransitionStatus::Stable,
            {"slide_left", "slide left", "slide"}
        },
        {
            std::string(ids::transition::slide_right),
            std::string(ids::transition::slide_right),
            "Slide Right",
            "Slide layer from right",
            TransitionKind::Slide,
            TransitionStatus::Stable,
            {"slide_right", "slide right"}
        },
        {
            std::string(ids::transition::zoom),
            std::string(ids::transition::zoom),
            "Zoom",
            "Zoom in/out transition",
            TransitionKind::Zoom,
            TransitionStatus::Stable,
            {"zoom", "scale", "zoom_in"}
        },
        {
            std::string(ids::transition::flip),
            std::string(ids::transition::flip),
            "Flip",
            "Flip transition",
            TransitionKind::Flip,
            TransitionStatus::Stable,
            {"flip", "flip horizontal", "flip_h"}
        },
        {
            std::string(ids::transition::blur),
            std::string(ids::transition::blur),
            "Blur",
            "Blur transition effect",
            TransitionKind::Custom, // Assuming Custom for complex shaders
            TransitionStatus::Stable,
            {"blur", "blur transition", "blur_in"}
        },
        {
            std::string(ids::transition::lightleak_amber_sweep),
            std::string(ids::transition::lightleak_amber_sweep),
            "Amber Sweep",
            "Cinematic light leak sweep",
            TransitionKind::Custom,
            TransitionStatus::Stable,
            {"amber_sweep", "lightleak_sweep"}
        }
    };
}

} // namespace tachyon
