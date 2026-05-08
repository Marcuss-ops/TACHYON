#include "tachyon/text/animation/text_animator.h"
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

    SelectionContext ctx;
    ctx.glyph_count = layout.glyphs.size();
    ctx.time = static_cast<float>(time);
    
    for (const auto& g : layout.glyphs) {
        ctx.word_count = std::max(ctx.word_count, g.word_index + 1);
        ctx.line_count = std::max(ctx.line_count, layout.lines.size());
    }

    for (const auto& anim : animators) {
        for (std::size_t i = 0; i < layout.glyphs.size(); ++i) {
            auto& glyph = layout.glyphs[i];
            
            float coverage = compute_coverage(ctx, anim.selector, glyph, i);
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

float TextAnimationSampler::compute_coverage(const SelectionContext& ctx,
                                             const TextAnimatorSelectorSpec& selector,
                                             const PositionedGlyph& glyph,
                                             std::size_t glyph_index) {
    float index = 0.0f;

    if (selector.based_on == "characters" || selector.based_on == "characters_excluding_spaces") {
        index = static_cast<float>(glyph_index);
    } else if (selector.based_on == "words") {
        index = static_cast<float>(glyph.word_index);
    } else if (selector.based_on == "lines") {
        // Line index would need to be calculated or passed in ctx
        index = 0.0f; 
    }

    float start_time = static_cast<float>(selector.offset);
    float time_since_start = ctx.time - start_time;
    
    if (time_since_start < 0.0f) {
        return 1.0f;
    }

    float glyph_delay = index * static_cast<float>(selector.stagger_delay);
    float progress = time_since_start - glyph_delay;
    float duration = 0.1f; 
    
    float factor = 1.0f - std::clamp(progress / duration, 0.0f, 1.0f);
    
    if (selector.shape == "square") {
        return progress >= 0.0f ? 0.0f : 1.0f;
    }
    
    return factor;
}

} // namespace tachyon::text
