#pragma once

namespace tachyon::renderer2d {

/// Linear color space RGB value (non-premultiplied alpha).
struct LinearColor {
    float r{0.0f};
    float g{0.0f};
    float b{0.0f};
};

/// Premultiplied alpha pixel (RGBA with alpha already multiplied into RGB channels).
struct PremultipliedPixel {
    float r{0.0f};
    float g{0.0f};
    float b{0.0f};
    float a{0.0f};
};

} // namespace tachyon::renderer2d