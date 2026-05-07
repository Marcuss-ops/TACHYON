#pragma once

#include "tachyon/core/spec/schema/animation/text_animator_spec.h"

namespace tachyon::text {

TextAnimatorSpec migrate_legacy_opacity_drop(float per_glyph_opacity_drop);

} // namespace tachyon::text
