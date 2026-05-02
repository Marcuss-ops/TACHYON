#pragma once

#include "tachyon/renderer2d/core/framebuffer.h"

namespace tachyon::renderer2d {

Color sample_texture_bilinear(const SurfaceRGBA& texture, float u, float v, Color tint = Color::white());

} // namespace tachyon::renderer2d
