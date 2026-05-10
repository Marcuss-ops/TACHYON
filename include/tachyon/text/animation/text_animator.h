#pragma once

#include "tachyon/text/layout/layout.h"
#include "tachyon/core/spec/schema/animation/text_animator_spec.h"
#include <vector>
#include <string>

namespace tachyon::text {

class TextAnimationSampler {
public:
    static void sample(TextLayoutResult& layout,
                       const std::vector<TextAnimatorSpec>& animators,
                       double time);
};


} // namespace tachyon::text
