#pragma once

#include "tachyon/renderer2d/core/framebuffer.h"
#include <vector>
#include <cstdint>

#include "tachyon/renderer2d/effects/color/color_types.h"

namespace tachyon::renderer2d {

std::vector<float> gaussian_kernel(float sigma);

SurfaceRGBA blur_surface(const SurfaceRGBA& input, float sigma);
SurfaceRGBA blur_alpha_mask(const SurfaceRGBA& input, float sigma);

// Low-level convolution (internal use)
std::vector<PremultipliedPixel> convolve_h(const std::vector<PremultipliedPixel>& in, std::uint32_t w, std::uint32_t h, const std::vector<float>& k);
std::vector<PremultipliedPixel> convolve_v(const std::vector<PremultipliedPixel>& in, std::uint32_t w, std::uint32_t h, const std::vector<float>& k);

} // namespace tachyon::renderer2d
