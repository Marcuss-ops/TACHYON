#pragma once

#include "tachyon/renderer2d/core/framebuffer.h"
#include <cstdint>

namespace tachyon::renderer2d {

struct GlitchEffect {
    float intensity{0.5f};      // 0..1
    float block_size{16.0f};    // px, dimensione dei blocchi
    uint64_t seed{0};           // per determinismo
    float scan_line_chance{0.3f};
    float rgb_shift_px{6.0f};
};

// Applica effetto glitch deterministico sul framebuffer
void apply_glitch(Framebuffer& fb, const GlitchEffect& params, int frame_number);

} // namespace tachyon::renderer2d
