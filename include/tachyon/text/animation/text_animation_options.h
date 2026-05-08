#pragma once

#include "tachyon/core/spec/schema/animation/text_animator_spec.h"

#include <span>

namespace tachyon::text {

struct TextAnimationOptions {
    // Master switch for animation evaluation. Kept on by default so existing
    // render paths continue to animate when animators are provided.
    bool enabled{true};

    // Runtime state shared across renderers: a local clock plus the animator set to evaluate.
    float time_seconds{0.0f};
    std::vector<::tachyon::TextAnimatorSpec> animators;

    // Motion blur intensity (0.0 = disabled, 1.0 = standard, >1.0 = exaggerated)
    float motion_blur_intensity{1.0f};
};

} // namespace tachyon::text
