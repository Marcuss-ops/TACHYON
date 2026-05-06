#include "tachyon/transition_catalog.h"

namespace tachyon {

std::vector<CatalogTransitionDescriptor> get_builtin_transition_descriptors() {
    return {
        {
            "tachyon.transition.fade",
            "tachyon.transition.fade",
            "Fade",
            "Crossfade between layers",
            TransitionKind::Fade,
            TransitionStatus::Stable,
            {"fade", "crossfade", "dissolve"}
        },
        {
            "tachyon.transition.slide_left",
            "tachyon.transition.slide_left",
            "Slide Left",
            "Slide layer from left",
            TransitionKind::Slide,
            TransitionStatus::Stable,
            {"slide_left", "slide left", "slide"}
        },
        {
            "tachyon.transition.slide_right",
            "tachyon.transition.slide_right",
            "Slide Right",
            "Slide layer from right",
            TransitionKind::Slide,
            TransitionStatus::Stable,
            {"slide_right", "slide right"}
        },
        {
            "tachyon.transition.zoom",
            "tachyon.transition.zoom",
            "Zoom",
            "Zoom in/out transition",
            TransitionKind::Zoom,
            TransitionStatus::Stable,
            {"zoom", "scale", "zoom_in"}
        },
        {
            "tachyon.transition.flip",
            "tachyon.transition.flip",
            "Flip",
            "Flip transition",
            TransitionKind::Flip,
            TransitionStatus::Stable,
            {"flip", "flip horizontal", "flip_h"}
        },
        {
            "tachyon.transition.blur",
            "tachyon.transition.blur",
            "Blur",
            "Blur transition effect",
            TransitionKind::Custom, // Assuming Custom for complex shaders
            TransitionStatus::Stable,
            {"blur", "blur transition", "blur_in"}
        },
        {
            "tachyon.transition.lightleak.amber_sweep",
            "tachyon.transition.lightleak.amber_sweep",
            "Amber Sweep",
            "Cinematic light leak sweep",
            TransitionKind::Custom,
            TransitionStatus::Stable,
            {"amber_sweep", "lightleak_sweep"}
        }
    };
}

} // namespace tachyon
