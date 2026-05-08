#pragma once

#include "tachyon/text/layout/layout.h"
#include "tachyon/core/spec/schema/animation/text_animator_spec.h"
#include <vector>
#include <string>

namespace tachyon::text {

/**
 * @brief Evaluation context for text selectors.
 */
struct SelectionContext {
    std::size_t glyph_count{0};
    std::size_t word_count{0};
    std::size_t line_count{0};
    
    // Normalized time (0..1) if the animator has a fixed duration
    float progress{0.0f};
    
    // Absolute time in seconds
    float time{0.0f};
};

/**
 * @brief Core engine for sampling text animations and applying them to a layout.
 */
class TextAnimationSampler {
public:
    /**
     * @brief Samples all animators for a given time and applies them to the layout.
     * 
     * @param layout The layout to animate (modified in-place)
     * @param animators The list of animator specifications
     * @param time Current time in seconds
     */
    static void sample(TextLayoutResult& layout,
                       const std::vector<TextAnimatorSpec>& animators,
                       double time);

    /**
     * @brief Computes the "coverage" (0..1) for a specific glyph based on a selector.
     */
    static float compute_coverage(const SelectionContext& ctx,
                                  const TextAnimatorSelectorSpec& selector,
                                  const PositionedGlyph& glyph,
                                  std::size_t glyph_index);
};

} // namespace tachyon::text
