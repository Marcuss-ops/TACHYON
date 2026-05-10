#include "tachyon/renderer2d/core/framebuffer.h"
#include <algorithm>
#include <cmath>
#include <omp.h>

namespace tachyon::renderer2d {

namespace {

inline float smoothstep01(float x) {
    return x * x * (3.0f - 2.0f * x);
}

// Optimized bilinear sampling with raw data access
inline void sample_bilinear_raw(const float* pixels, uint32_t width, uint32_t height, float u, float v, float* out_rgba) {
    const float fw = static_cast<float>(width);
    const float fh = static_cast<float>(height);
    
    float x = u * fw - 0.5f;
    float y = v * fh - 0.5f;
    
    int x0 = static_cast<int>(std::floor(x));
    int y0 = static_cast<int>(std::floor(y));
    
    float dx = x - static_cast<float>(x0);
    float dy = y - static_cast<float>(y0);
    
    x0 = std::clamp(x0, 0, static_cast<int>(width - 1));
    y0 = std::clamp(y0, 0, static_cast<int>(height - 1));
    int x1 = std::min(x0 + 1, static_cast<int>(width - 1));
    int y1 = std::min(y0 + 1, static_cast<int>(height - 1));
    
    const float* p00 = &pixels[(y0 * width + x0) * 4];
    const float* p10 = &pixels[(y0 * width + x1) * 4];
    const float* p01 = &pixels[(y1 * width + x0) * 4];
    const float* p11 = &pixels[(y1 * width + x1) * 4];
    
    const float inv_dx = 1.0f - dx;
    const float inv_dy = 1.0f - dy;
    
    const float w00 = inv_dx * inv_dy;
    const float w10 = dx * inv_dy;
    const float w01 = inv_dx * dy;
    const float w11 = dx * dy;
    
    out_rgba[0] = p00[0] * w00 + p10[0] * w10 + p01[0] * w01 + p11[0] * w11;
    out_rgba[1] = p00[1] * w00 + p10[1] * w10 + p01[1] * w01 + p11[1] * w11;
    out_rgba[2] = p00[2] * w00 + p10[2] * w10 + p01[2] * w01 + p11[2] * w11;
    out_rgba[3] = p00[3] * w00 + p10[3] * w10 + p01[3] * w01 + p11[3] * w11;
}

} // namespace

void apply_soft_zoom_blur_fused_direct(
    SurfaceRGBA& output,
    const SurfaceRGBA& from,
    const SurfaceRGBA* to,
    float progress,
    int thread_count) {

    const uint32_t width = output.width();
    const uint32_t height = output.height();
    const float inv_width = 1.0f / static_cast<float>(width);
    const float inv_height = 1.0f / static_cast<float>(height);

    const float* from_data = from.pixels().data();
    const float* to_data = to ? to->pixels().data() : nullptr;
    float* out_data = output.mutable_pixels().data();

    constexpr int samples = 7;
    const float eased = smoothstep01(progress);
    const float inv_eased = 1.0f - eased;
    const float blur_phase = 0.12f * std::sin(eased * 3.1415926535f);
    const float cx = 0.5f;
    const float cy = 0.5f;

    #pragma omp parallel for num_threads(thread_count) schedule(dynamic, 16)
    for (int y = 0; y < static_cast<int>(height); ++y) {
        for (int x = 0; x < static_cast<int>(width); ++x) {
            const float u = (static_cast<float>(x) + 0.5f) * inv_width;
            const float v = (static_cast<float>(y) + 0.5f) * inv_height;

            float acc_r = 0.0f, acc_g = 0.0f, acc_b = 0.0f, acc_a = 0.0f;

            for (int i = 0; i < samples; ++i) {
                const float s = static_cast<float>(i) / static_cast<float>(samples - 1);
                const float blur = (s - 0.5f) * blur_phase;

                const float su = cx + (u - cx) * (1.0f + blur);
                const float sv = cy + (v - cy) * (1.0f + blur);

                float src[4];
                sample_bilinear_raw(from_data, from.width(), from.height(), su, sv, src);
                
                float dst[4] = {0.0f, 0.0f, 0.0f, 0.0f};
                if (to_data) {
                    sample_bilinear_raw(to_data, to->width(), to->height(), su, sv, dst);
                }

                // Inline lerp and accumulation
                acc_r += (src[0] * inv_eased + dst[0] * eased);
                acc_g += (src[1] * inv_eased + dst[1] * eased);
                acc_b += (src[2] * inv_eased + dst[2] * eased);
                acc_a += (src[3] * inv_eased + dst[3] * eased);
            }

            const float inv_samples = 1.0f / static_cast<float>(samples);
            float* out_pixel = &out_data[(y * width + x) * 4];
            out_pixel[0] = acc_r * inv_samples;
            out_pixel[1] = acc_g * inv_samples;
            out_pixel[2] = acc_b * inv_samples;
            out_pixel[3] = acc_a * inv_samples;
        }
    }
}


} // namespace tachyon::renderer2d
