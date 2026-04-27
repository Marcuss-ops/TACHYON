#include "tachyon/renderer2d/effects/effect_host.h"
#include "tachyon/renderer2d/effects/effect_utils.h"

#include <cmath>
#include <array>

namespace tachyon::renderer2d {
namespace {

// Invert a 3x3 homography matrix
std::array<float, 9> invert_homography(const std::array<float, 9>& h) {
    std::array<float, 9> inv;
    
    // Compute determinant
    float det = h[0] * (h[4] * h[8] - h[5] * h[7]) -
                h[1] * (h[3] * h[8] - h[5] * h[6]) +
                h[2] * (h[3] * h[7] - h[4] * h[6]);
    
    if (std::abs(det) < 1e-10f) {
        // Singular matrix, return identity
        return {1,0,0, 0,1,0, 0,0,1};
    }
    
    float inv_det = 1.0f / det;
    
    inv[0] = (h[4] * h[8] - h[5] * h[7]) * inv_det;
    inv[1] = (h[2] * h[7] - h[1] * h[8]) * inv_det;
    inv[2] = (h[1] * h[5] - h[2] * h[4]) * inv_det;
    inv[3] = (h[5] * h[6] - h[3] * h[8]) * inv_det;
    inv[4] = (h[0] * h[8] - h[2] * h[6]) * inv_det;
    inv[5] = (h[2] * h[3] - h[0] * h[5]) * inv_det;
    inv[6] = (h[3] * h[7] - h[4] * h[6]) * inv_det;
    inv[7] = (h[1] * h[6] - h[0] * h[7]) * inv_det;
    inv[8] = (h[0] * h[4] - h[1] * h[3]) * inv_det;
    
    return inv;
}

// Bilinear interpolation
Color bilinear_sample(const SurfaceRGBA& src, float x, float y) {
    int x0 = static_cast<int>(std::floor(x));
    int y0 = static_cast<int>(std::floor(y));
    int x1 = x0 + 1;
    int y1 = y0 + 1;
    
    float fx = x - x0;
    float fy = y - y0;
    
    // Clamp coordinates
    x0 = std::max(0, std::min(static_cast<int>(src.width()) - 1, x0));
    x1 = std::max(0, std::min(static_cast<int>(src.width()) - 1, x1));
    y0 = std::max(0, std::min(static_cast<int>(src.height()) - 1, y0));
    y1 = std::max(0, std::min(static_cast<int>(src.height()) - 1, y1));
    
    Color p00 = src.get_pixel(static_cast<std::uint32_t>(x0), static_cast<std::uint32_t>(y0));
    Color p10 = src.get_pixel(static_cast<std::uint32_t>(x1), static_cast<std::uint32_t>(y0));
    Color p01 = src.get_pixel(static_cast<std::uint32_t>(x0), static_cast<std::uint32_t>(y1));
    Color p11 = src.get_pixel(static_cast<std::uint32_t>(x1), static_cast<std::uint32_t>(y1));
    
    Color result;
    result.r = (1.0f - fx) * (1.0f - fy) * p00.r + fx * (1.0f - fy) * p10.r + 
                (1.0f - fx) * fy * p01.r + fx * fy * p11.r;
    result.g = (1.0f - fx) * (1.0f - fy) * p00.g + fx * (1.0f - fy) * p10.g + 
                (1.0f - fx) * fy * p01.g + fx * fy * p11.g;
    result.b = (1.0f - fx) * (1.0f - fy) * p00.b + fx * (1.0f - fy) * p10.b + 
                (1.0f - fx) * fy * p01.b + fx * fy * p11.b;
    result.a = (1.0f - fx) * (1.0f - fy) * p00.a + fx * (1.0f - fy) * p10.a + 
                (1.0f - fx) * fy * p01.a + fx * fy * p11.a;
    
    return result;
}

} // namespace

SurfaceRGBA WarpStabilizerEffect::apply(const SurfaceRGBA& input, const EffectParams& params) const {
    // Read homography matrix (3x3 = 9 values)
    std::array<float, 9> H;
    for (int i = 0; i < 9; ++i) {
        std::string key = "h0" + std::to_string(i);
        H[i] = get_scalar(params, key.c_str(), (i % 4 == 0) ? 1.0f : 0.0f);
    }
    
    // Invert the homography for inverse warp
    std::array<float, 9> inv_H = invert_homography(H);
    
    SurfaceRGBA out = input;
    out.clear(Color::transparent());
    
    // Apply inverse warp: for each output pixel, compute source position
    for (std::uint32_t y = 0; y < out.height(); ++y) {
        for (std::uint32_t x = 0; x < out.width(); ++x) {
            float nx = (x + 0.5f) / out.width();
            float ny = (y + 0.5f) / out.height();
            
            // Apply inverse homography
            float denom = inv_H[6] * nx + inv_H[7] * ny + inv_H[8];
            if (std::abs(denom) < 1e-10f) continue;
            
            float src_x = (inv_H[0] * nx + inv_H[1] * ny + inv_H[2]) / denom;
            float src_y = (inv_H[3] * nx + inv_H[4] * ny + inv_H[5]) / denom;
            
            // Convert back to pixel coordinates
            src_x = (src_x * out.width()) - 0.5f;
            src_y = (src_y * out.height()) - 0.5f;
            
            // Sample with bilinear interpolation
            Color sampled = bilinear_sample(input, src_x, src_y);
            out.set_pixel(x, y, sampled);
        }
    }
    
    return out;
}

} // namespace tachyon::renderer2d
