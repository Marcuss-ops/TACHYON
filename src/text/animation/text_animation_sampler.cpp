#include "tachyon/text/animation/text_animator.h"
#include "tachyon/text/animation/text_animator_utils.h"

#include <algorithm>
#include <cmath>

namespace tachyon::text {

namespace {

float apply_shape(float progress, const std::string& shape) {
    if (shape == "square") {
        return progress >= 1.0f ? 1.0f : 0.0f;
    }
    if (shape == "ramp_up") {
        return std::clamp(progress, 0.0f, 1.0f);
    }
    if (shape == "ramp_down") {
        return 1.0f - std::clamp(progress, 0.0f, 1.0f);
    }
    return std::clamp(progress, 0.0f, 1.0f);
}

} // anonymous namespace

void TextAnimationSampler::sample(TextLayoutResult& layout,
                                  const std::vector<TextAnimatorSpec>& animators,
                                  double time) {
    if (layout.glyphs.empty()) return;

    TextAnimatorContext ctx;
    ctx.time = static_cast<float>(time);
    ctx.total_glyphs = static_cast<float>(layout.glyphs.size());
    
    // Count words and lines
    std::size_t max_word = 0;
    for (const auto& g : layout.glyphs) {
        max_word = std::max(max_word, g.word_index);
    }
    ctx.total_words = static_cast<float>(max_word + 1);
    ctx.total_lines = static_cast<float>(layout.lines.size());

    for (const auto& anim : animators) {
        for (std::size_t i = 0; i < layout.glyphs.size(); ++i) {
            auto& glyph = layout.glyphs[i];
            
            ctx.glyph_index = i;
            ctx.word_index = glyph.word_index;
            ctx.line_index = glyph.line_index;
            ctx.is_space = (glyph.codepoint == ' ' || glyph.codepoint == '\t' || glyph.codepoint == '\n');
            
            float coverage = compute_coverage(anim.selector, ctx);
            if (coverage <= 0.0f) continue;

            if (anim.properties.opacity_value.has_value()) {
                float target = static_cast<float>(*anim.properties.opacity_value);
                glyph.opacity *= (1.0f - coverage) + (target * coverage);
            }
            
            if (anim.properties.reveal_value.has_value()) {
                float target = static_cast<float>(*anim.properties.reveal_value);
                glyph.reveal_factor = (1.0f - coverage) + (target * coverage);
            }

            if (anim.properties.position_offset_value.has_value()) {
                auto offset = *anim.properties.position_offset_value;
                glyph.position.x += offset.x * coverage;
                glyph.position.y += offset.y * coverage;
            }

            if (anim.properties.scale_value.has_value()) {
                float s = static_cast<float>(*anim.properties.scale_value);
                glyph.scale.x *= (1.0f - coverage) + (s * coverage);
                glyph.scale.y *= (1.0f - coverage) + (s * coverage);
            }
            
            if (anim.properties.blur_radius_value.has_value()) {
                float target = static_cast<float>(*anim.properties.blur_radius_value);
                glyph.blur_radius += target * coverage;
            }
        }
    }
}


} // namespace tachyon::text
