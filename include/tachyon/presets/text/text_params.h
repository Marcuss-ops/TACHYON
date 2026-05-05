#pragma once

#include "tachyon/core/spec/schema/animation/text_animator_spec.h"
#include "tachyon/presets/preset_base.h"
#include "tachyon/core/types/colors.h"
#include <cstdint>
#include <string>
#include <vector>

namespace tachyon::presets {

struct TextParams : LayerParams {
    std::string text{"Hello World"};
    std::string font_id;
    uint32_t    font_size{72};
    std::string alignment{"center"};
    ColorSpec   color{238, 242, 248, 245};

    // Animation
    std::string animation{"tachyon.textanim.fade_up"};
    double      reveal_duration{0.45};
    double      stagger_delay{0.03};

    // Text box — separate from layer position/size in LayerParams
    float text_w{1440.0f};
    float text_h{220.0f};

    // Typed animators (fluent API)
    std::vector<tachyon::TextAnimatorSpec> animations;
};

} // namespace tachyon::presets
