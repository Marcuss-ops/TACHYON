#include "tachyon/text/core/animation/text_animator_utils.h"
#include <vector>

bool run_text_stagger_tests() {
    using namespace tachyon::text;
    
    // Create test glyphs with stagger effect
    std::vector<Glyph> glyphs(5);
    for (size_t i = 0; i < glyphs.size(); ++i) {
        glyphs[i].opacity = 1.0f;
    }
    
    // Apply stagger effect (each subsequent glyph delayed)
    apply_stagger(glyphs, 0.1f); // 0.1s stagger per glyph
    
    // Verify adjacent glyphs have different opacities at a given time
    for (size_t i = 0; i < glyphs.size() - 1; ++i) {
        if (std::abs(glyphs[i].opacity - glyphs[i+1].opacity) < 0.001f) {
            return false;
        }
    }
    
    return true;
}
