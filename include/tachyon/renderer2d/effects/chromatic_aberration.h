#pragma once

#include "tachyon/renderer2d/core/framebuffer.h"

namespace tachyon::renderer2d {

struct ChromaticAberrationEffect {
    float offset_pixels{4.0f};  // quanto si separano i canali RGB
    float angle_deg{0.0f};      // direzione della separazione
};

// Applica sull'intera Framebuffer dopo il composite:
void apply_chromatic_aberration(Framebuffer& fb, const ChromaticAberrationEffect& params);

} // namespace tachyon::renderer2d
