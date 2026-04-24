#pragma once

#include "tachyon/text/core/layout/resolved_text_layout.h"
#include "tachyon/core/spec/schema/animation/text_animator_spec.h"
#include <cmath>

namespace tachyon::text {

struct TypewriterOptions {
    float chars_per_second{30.0f};   // Speed of reveal
    float start_time{0.0f};           // When the effect starts
    bool show_cursor{false};          // Whether to show a cursor
    float cursor_blink_hz{2.0f};     // Cursor blink frequency
    float cursor_opacity{0.75f};     // Cursor opacity
};

class TypewriterAnimator {
public:
    /**
     * @brief Applies a typewriter effect to the text layout.
     * Reveals characters progressively based on time.
     * 
     * @param layout The text layout to animate (modified in-place)
     * @param options Configuration for the typewriter effect
     * @param time Current time in seconds
     */
    static void apply(ResolvedTextLayout& layout,
                      const TypewriterOptions& options,
                      float time) {
        if (layout.glyphs.empty()) return;

        float elapsed = time - options.start_time;
        if (elapsed <= 0.0f) {
            // Hide all glyphs
            for (auto& glyph : layout.glyphs) {
                glyph.opacity = 0.0f;
                glyph.reveal_factor = 0.0f;
            }
            return;
        }

        // Calculate how many characters should be visible
        float visible_count = elapsed * options.chars_per_second;
        float total_glyphs = static_cast<float>(layout.glyphs.size());

        for (std::size_t i = 0; i < layout.glyphs.size(); ++i) {
            auto& glyph = layout.glyphs[i];
            float glyph_index = static_cast<float>(i);

            if (glyph_index < visible_count) {
                // Fully revealed
                glyph.opacity = 1.0f;
                glyph.reveal_factor = 1.0f;
            } else if (glyph_index - visible_count < 1.0f) {
                // Partially revealed (smooth transition)
                float partial = visible_count - glyph_index;
                glyph.opacity = std::clamp(partial, 0.0f, 1.0f);
                glyph.reveal_factor = glyph.opacity;
            } else {
                // Not yet revealed
                glyph.opacity = 0.0f;
                glyph.reveal_factor = 0.0f;
            }
        }

        // Handle cursor
        if (options.show_cursor && visible_count < total_glyphs) {
            std::size_t cursor_index = static_cast<std::size_t>(std::floor(visible_count));
            if (cursor_index < layout.glyphs.size()) {
                float blink = std::sin(2.0f * 3.14159265358979323846f * options.cursor_blink_hz * time);
                float cursor_alpha = options.cursor_opacity * (0.5f + 0.5f * blink);
                
                auto& glyph = layout.glyphs[cursor_index];
                // Cursor is implied by modifying the glyph after the revealed one
                // In practice, the renderer would draw a cursor shape
                glyph.cursor_visible = true;
                glyph.cursor_opacity = cursor_alpha;
            }
        }
    }

    /**
     * @brief Applies typewriter effect from animator spec.
     */
    static void apply_from_spec(ResolvedTextLayout& layout,
                                const TextAnimatorSpec& spec,
                                float time) {
        TypewriterOptions options;
        options.chars_per_second = static_cast<float>(spec.properties.reveal_value);
        options.start_time = 0.0f; // Would come from spec
        options.show_cursor = false;
        
        apply(layout, options, time);
    }
};

} // namespace tachyon::text
