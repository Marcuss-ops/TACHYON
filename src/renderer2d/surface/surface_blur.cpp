#include "tachyon/renderer2d/surface/surface_blur.h"
#include <vector>
#include <algorithm>
#include <cmath>

#ifdef _OPENMP
#include <omp.h>
#endif

namespace tachyon::renderer2d {

namespace {

// Fast separable box blur implementation (linear time O(N))
void box_blur_h(const float* src, float* dst, int w, int h, int r) {
    float iarr = 1.0f / (r + r + 1.0f);
    for (int i = 0; i < h; i++) {
        int ti = i * w * 4;
        int li = ti;
        int ri = ti + r * 4;
        
        float fvr = src[ti + 0];
        float fvg = src[ti + 1];
        float fvb = src[ti + 2];
        float fva = src[ti + 3];
        
        float lvr = src[ti + (w - 1) * 4 + 0];
        float lvg = src[ti + (w - 1) * 4 + 1];
        float lvb = src[ti + (w - 1) * 4 + 2];
        float lva = src[ti + (w - 1) * 4 + 3];
        
        float valr = (r + 1) * fvr;
        float valg = (r + 1) * fvg;
        float valb = (r + 1) * fvb;
        float vala = (r + 1) * fva;
        
        for (int j = 0; j < r; j++) {
            valr += src[ti + j * 4 + 0];
            valg += src[ti + j * 4 + 1];
            valb += src[ti + j * 4 + 2];
            vala += src[ti + j * 4 + 3];
        }
        
        for (int j = 0; j <= r; j++) {
            valr += src[ri + 0] - fvr;
            valg += src[ri + 1] - fvg;
            valb += src[ri + 2] - fvb;
            vala += src[ri + 3] - fva;
            dst[ti + 0] = valr * iarr;
            dst[ti + 1] = valg * iarr;
            dst[ti + 2] = valb * iarr;
            dst[ti + 3] = vala * iarr;
            ri += 4; ti += 4;
        }
        
        for (int j = r + 1; j < w - r; j++) {
            valr += src[ri + 0] - src[li + 0];
            valg += src[ri + 1] - src[li + 1];
            valb += src[ri + 2] - src[li + 2];
            vala += src[ri + 3] - src[li + 3];
            dst[ti + 0] = valr * iarr;
            dst[ti + 1] = valg * iarr;
            dst[ti + 2] = valb * iarr;
            dst[ti + 3] = vala * iarr;
            li += 4; ri += 4; ti += 4;
        }
        
        for (int j = w - r; j < w; j++) {
            valr += lvr - src[li + 0];
            valg += lvg - src[li + 1];
            valb += lvb - src[li + 2];
            vala += lva - src[li + 3];
            dst[ti + 0] = valr * iarr;
            dst[ti + 1] = valg * iarr;
            dst[ti + 2] = valb * iarr;
            dst[ti + 3] = vala * iarr;
            li += 4; ti += 4;
        }
    }
}

void box_blur_v(const float* src, float* dst, int w, int h, int r) {
    float iarr = 1.0f / (r + r + 1.0f);
    for (int i = 0; i < w; i++) {
        int ti = i * 4;
        int li = ti;
        int ri = ti + r * w * 4;
        
        float fvr = src[ti + 0];
        float fvg = src[ti + 1];
        float fvb = src[ti + 2];
        float fva = src[ti + 3];
        
        float lvr = src[ti + (h - 1) * w * 4 + 0];
        float lvg = src[ti + (h - 1) * w * 4 + 1];
        float lvb = src[ti + (h - 1) * w * 4 + 2];
        float lva = src[ti + (h - 1) * w * 4 + 3];
        
        float valr = (r + 1) * fvr;
        float valg = (r + 1) * fvg;
        float valb = (r + 1) * fvb;
        float vala = (r + 1) * fva;
        
        for (int j = 0; j < r; j++) {
            valr += src[ti + j * w * 4 + 0];
            valg += src[ti + j * w * 4 + 1];
            valb += src[ti + j * w * 4 + 2];
            vala += src[ti + j * w * 4 + 3];
        }
        
        for (int j = 0; j <= r; j++) {
            valr += src[ri + 0] - fvr;
            valg += src[ri + 1] - fvg;
            valb += src[ri + 2] - fvb;
            vala += src[ri + 3] - fva;
            dst[ti + 0] = valr * iarr;
            dst[ti + 1] = valg * iarr;
            dst[ti + 2] = valb * iarr;
            dst[ti + 3] = vala * iarr;
            ri += w * 4; ti += w * 4;
        }
        
        for (int j = r + 1; j < h - r; j++) {
            valr += src[ri + 0] - src[li + 0];
            valg += src[ri + 1] - src[li + 1];
            valb += src[ri + 2] - src[li + 2];
            vala += src[ri + 3] - src[li + 3];
            dst[ti + 0] = valr * iarr;
            dst[ti + 1] = valg * iarr;
            dst[ti + 2] = valb * iarr;
            dst[ti + 3] = vala * iarr;
            li += w * 4; ri += w * 4; ti += w * 4;
        }
        
        for (int j = h - r; j < h; j++) {
            valr += lvr - src[li + 0];
            valg += lvg - src[li + 1];
            valb += lvb - src[li + 2];
            vala += lva - src[li + 3];
            dst[ti + 0] = valr * iarr;
            dst[ti + 1] = valg * iarr;
            dst[ti + 2] = valb * iarr;
            dst[ti + 3] = vala * iarr;
            li += w * 4; ti += w * 4;
        }
    }
}

} // namespace

void apply_box_blur(SurfaceRGBA& surface, int radius, int thread_count) {
    if (radius <= 0) return;
    int w = static_cast<int>(surface.width());
    int h = static_cast<int>(surface.height());
    radius = std::min(radius, std::min(w, h) / 2 - 1);
    
    std::vector<float> temp_pixels(surface.pixels().size());
    float* pixels = surface.mutable_pixels().data();
    float* temp = temp_pixels.data();
    
    // Horizontal pass
    box_blur_h(pixels, temp, w, h, radius);
    // Vertical pass
    box_blur_v(temp, pixels, w, h, radius);
}

void apply_fast_gaussian_blur(SurfaceRGBA& surface, float sigma, int passes, int thread_count) {
    if (sigma <= 0.0f) return;
    
    // Heuristic: convert sigma to box radius
    // For 3 passes, it approximates a Gaussian with sigma = sqrt(n*(r*r-1)/12)
    // We simplify for performance.
    int radius = static_cast<int>(std::round(sigma * 1.5f)); 
    if (radius < 1) radius = 1;
    
    for (int i = 0; i < passes; ++i) {
        apply_box_blur(surface, radius, thread_count);
    }
}

} // namespace tachyon::renderer2d
