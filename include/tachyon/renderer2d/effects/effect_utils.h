#pragma once

#include "tachyon/renderer2d/effects/effect_common.h"

namespace tachyon::renderer2d {

namespace detail {

using LinearColor = tachyon::renderer2d::LinearColor;
using PremultipliedPixel = tachyon::renderer2d::PremultipliedPixel;

// Import functions from the main namespace for use in detail implementations
using ::tachyon::renderer2d::clamp01;
using ::tachyon::renderer2d::lerp;
using ::tachyon::renderer2d::lerp_color;

using ::tachyon::renderer2d::has_scalar;
using ::tachyon::renderer2d::has_color;
using ::tachyon::renderer2d::get_scalar;
using ::tachyon::renderer2d::get_color;

using ::tachyon::renderer2d::to_linear;
using ::tachyon::renderer2d::from_linear;
using ::tachyon::renderer2d::luminance;

using ::tachyon::renderer2d::rgb_to_hsl;
using ::tachyon::renderer2d::hsl_to_rgb;

using ::tachyon::renderer2d::to_premultiplied;
using ::tachyon::renderer2d::from_premultiplied;

using ::tachyon::renderer2d::sample_texture_bilinear;

using ::tachyon::renderer2d::gaussian_kernel;
using ::tachyon::renderer2d::convolve_h;
using ::tachyon::renderer2d::convolve_v;

using ::tachyon::renderer2d::blur_surface;
using ::tachyon::renderer2d::blur_alpha_mask;
using ::tachyon::renderer2d::composite_with_offset;

using ::tachyon::renderer2d::build_channel_lut;
using ::tachyon::renderer2d::apply_channel_lut;

template <typename Fn>
SurfaceRGBA transform_surface(const SurfaceRGBA& input, Fn&& fn) {
    SurfaceRGBA output(input.width(), input.height());
    for (std::uint32_t y = 0; y < input.height(); ++y) {
        for (std::uint32_t x = 0; x < input.width(); ++x) {
            output.set_pixel(x, y, fn(input.get_pixel(x, y)));
        }
    }
    return output;
}

} // namespace detail
} // namespace tachyon::renderer2d
