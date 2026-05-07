#include "tachyon/text/animation/text_anim_migration.h"

namespace tachyon::text {

TextAnimatorSpec migrate_legacy_opacity_drop(float per_glyph_opacity_drop) {
    // Legacy opacity drop is now handled as pre-processing in apply_text_animators.
    // This function is kept for API compatibility but returns an empty spec.
    (void)per_glyph_opacity_drop;
    TextAnimatorSpec spec;
    spec.name = "legacy_opacity_drop_migrated";
    spec.selector.type = "none";
    return spec;
}

} // namespace tachyon::text
