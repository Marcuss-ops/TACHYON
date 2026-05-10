#include "tachyon/renderer2d/core/framebuffer.h"
#include "tachyon/core/transition/transition_simd_kernels.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <omp.h>
#include <iostream>

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


void apply_crossfade_fused_direct(
    SurfaceRGBA& output,
    const SurfaceRGBA& from,
    const SurfaceRGBA* to,
    float progress,
    int thread_count) {

    const uint32_t width = output.width();
    const uint32_t height = output.height();
    
    // Fast path: dimensions match exactly - use SIMD
    if (to && to->width() == width && to->height() == height && from.width() == width && from.height() == height) {
        std::cout << "[debug] crossfade FAST PATH hit (" << width << "x" << height << ")" << std::endl;
        const float t = std::clamp(progress, 0.0f, 1.0f);
        const float* from_data = from.pixels().data();
        const float* to_data = to->pixels().data();
        float* out_data = output.mutable_pixels().data();
        
        #pragma omp parallel for num_threads(thread_count) schedule(static)
        for (int y = 0; y < static_cast<int>(height); ++y) {
            const std::size_t offset = static_cast<std::size_t>(y) * width * 4;
            tachyon::runtime::simd::lerp_pixels_best(
                out_data + offset,
                from_data + offset,
                to_data + offset,
                static_cast<std::size_t>(width) * 4,
                t);
        }
        return;
    }

    std::cout << "[debug] crossfade FALLBACK hit (mismatch: out=" << width << "x" << height 
              << " from=" << from.width() << "x" << from.height() 
              << " to=" << (to ? to->width() : 0) << "x" << (to ? to->height() : 0) << ")" << std::endl;

    // Fallback: Bilinear sampling for mismatched sizes
    const float* from_data = from.pixels().data();
    const float* to_data = to ? to->pixels().data() : nullptr;
    float* out_data = output.mutable_pixels().data();
    const float inv_width = 1.0f / static_cast<float>(width);
    const float inv_height = 1.0f / static_cast<float>(height);

    #pragma omp parallel for num_threads(thread_count) schedule(dynamic, 16)
    for (int y = 0; y < static_cast<int>(height); ++y) {
        for (int x = 0; x < static_cast<int>(width); ++x) {
            const float u = (static_cast<float>(x) + 0.5f) * inv_width;
            const float v = (static_cast<float>(y) + 0.5f) * inv_height;
            
            float src[4];
            sample_bilinear_raw(from_data, from.width(), from.height(), u, v, src);
            
            float dst[4] = {0.0f, 0.0f, 0.0f, 1.0f};
            if (to_data) {
                sample_bilinear_raw(to_data, to->width(), to->height(), u, v, dst);
            }
            
            float* out_pixel = &out_data[(y * width + x) * 4];
            out_pixel[0] = src[0] * (1.0f - progress) + dst[0] * progress;
            out_pixel[1] = src[1] * (1.0f - progress) + dst[1] * progress;
            out_pixel[2] = src[2] * (1.0f - progress) + dst[2] * progress;
            out_pixel[3] = src[3] * (1.0f - progress) + dst[3] * progress;
        }
    }
}

void apply_slide_fused_direct(
    SurfaceRGBA& output,
    const SurfaceRGBA& from,
    const SurfaceRGBA* to,
    float progress,
    int thread_count) {

    const uint32_t width = output.width();
    const uint32_t height = output.height();
    
    // Fast path: dimensions match exactly - use Row-based memcpy
    if (to && to->width() == width && to->height() == height && from.width() == width && from.height() == height) {
        std::cout << "[debug] slide FAST PATH hit (" << width << "x" << height << ")" << std::endl;
        const float t = std::clamp(progress, 0.0f, 1.0f);
        const int offset_x = static_cast<int>(t * static_cast<float>(width));
        
        const float* from_data = from.pixels().data();
        const float* to_data = to->pixels().data();
        float* out_data = output.mutable_pixels().data();
        
        #pragma omp parallel for num_threads(thread_count) schedule(static)
        for (int y = 0; y < static_cast<int>(height); ++y) {
            const std::size_t row_offset = static_cast<std::size_t>(y) * width * 4;
            
            // Source part (moves to the left)
            if (offset_x < static_cast<int>(width)) {
                const int remaining_width = static_cast<int>(width) - offset_x;
                std::memcpy(
                    out_data + row_offset, 
                    from_data + row_offset + (static_cast<std::size_t>(offset_x) * 4), 
                    static_cast<std::size_t>(remaining_width) * 4 * sizeof(float)
                );
            }
            
            // Target part (comes from the right)
            if (offset_x > 0) {
                const int target_dest_offset = static_cast<int>(width) - offset_x;
                std::memcpy(
                    out_data + row_offset + (static_cast<std::size_t>(target_dest_offset) * 4), 
                    to_data + row_offset, 
                    static_cast<std::size_t>(offset_x) * 4 * sizeof(float)
                );
            }
        }
        return;
    }

    std::cout << "[debug] slide FALLBACK hit" << std::endl;

    // Fallback: Bilinear sampling for mismatched sizes
    const float* from_data = from.pixels().data();
    const float* to_data = to ? to->pixels().data() : nullptr;
    float* out_data = output.mutable_pixels().data();
    const float inv_width = 1.0f / static_cast<float>(width);
    const float inv_height = 1.0f / static_cast<float>(height);

    #pragma omp parallel for num_threads(thread_count) schedule(dynamic, 16)
    for (int y = 0; y < static_cast<int>(height); ++y) {
        for (int x = 0; x < static_cast<int>(width); ++x) {
            const float u = (static_cast<float>(x) + 0.5f) * inv_width;
            const float v = (static_cast<float>(y) + 0.5f) * inv_height;
            
            const float su = u + progress;
            float src[4];
            sample_bilinear_raw(from_data, from.width(), from.height(), su, v, src);
            
            const float tu = u + progress - 1.0f;
            float dst[4] = {0.0f, 0.0f, 0.0f, 1.0f};
            if (to_data) {
                sample_bilinear_raw(to_data, to->width(), to->height(), tu, v, dst);
            }
            
            float* out_pixel = &out_data[(y * width + x) * 4];
            out_pixel[0] = src[0] * (1.0f - progress) + dst[0] * progress;
            out_pixel[1] = src[1] * (1.0f - progress) + dst[1] * progress;
            out_pixel[2] = src[2] * (1.0f - progress) + dst[2] * progress;
            out_pixel[3] = src[3] * (1.0f - progress) + dst[3] * progress;
        }
    }
}


void apply_wipe_linear_fused_direct(
    SurfaceRGBA& output,
    const SurfaceRGBA& from,
    const SurfaceRGBA* to,
    float progress,
    int thread_count) {

    const uint32_t width = output.width();
    const uint32_t height = output.height();
    const float* from_data = from.pixels().data();
    const float* to_data = to ? to->pixels().data() : nullptr;
    float* out_data = output.mutable_pixels().data();

    #pragma omp parallel for num_threads(thread_count) schedule(static)
    for (int y = 0; y < static_cast<int>(height); ++y) {
        for (int x = 0; x < static_cast<int>(width); ++x) {
            const float u = (static_cast<float>(x) + 0.5f) / static_cast<float>(width);
            const std::size_t offset = (static_cast<std::size_t>(y) * width + x) * 4;
            
            if (u < progress && to_data) {
                std::memcpy(out_data + offset, to_data + offset, 4 * sizeof(float));
            } else {
                std::memcpy(out_data + offset, from_data + offset, 4 * sizeof(float));
            }
        }
    }
}

void apply_smooth_wipe_fused_direct(
    SurfaceRGBA& output,
    const SurfaceRGBA& from,
    const SurfaceRGBA* to,
    float progress,
    int thread_count) {

    const uint32_t width = output.width();
    const uint32_t height = output.height();
    const float* from_data = from.pixels().data();
    const float* to_data = to ? to->pixels().data() : nullptr;
    float* out_data = output.mutable_pixels().data();
    const float smoothness = 0.1f;

    #pragma omp parallel for num_threads(thread_count) schedule(static)
    for (int y = 0; y < static_cast<int>(height); ++y) {
        for (int x = 0; x < static_cast<int>(width); ++x) {
            const float u = (static_cast<float>(x) + 0.5f) / static_cast<float>(width);
            const std::size_t offset = (static_cast<std::size_t>(y) * width + x) * 4;
            
            float edge = std::clamp((progress - u) / smoothness + 0.5f, 0.0f, 1.0f);
            float eased_edge = smoothstep01(edge);
            
            const float* src = from_data + offset;
            const float* dst = to_data ? (to_data + offset) : src;
            float* out = out_data + offset;

            out[0] = src[0] * (1.0f - eased_edge) + dst[0] * eased_edge;
            out[1] = src[1] * (1.0f - eased_edge) + dst[1] * eased_edge;
            out[2] = src[2] * (1.0f - eased_edge) + dst[2] * eased_edge;
            out[3] = src[3] * (1.0f - eased_edge) + dst[3] * eased_edge;
        }
    }
}


void apply_circle_iris_fused_direct(
    SurfaceRGBA& output,
    const SurfaceRGBA& from,
    const SurfaceRGBA* to,
    float progress,
    int thread_count) {

    const uint32_t width = output.width();
    const uint32_t height = output.height();
    const float* from_data = from.pixels().data();
    const float* to_data = to ? to->pixels().data() : nullptr;
    float* out_data = output.mutable_pixels().data();
    const float aspect = static_cast<float>(width) / static_cast<float>(height);

    #pragma omp parallel for num_threads(thread_count) schedule(static)
    for (int y = 0; y < static_cast<int>(height); ++y) {
        for (int x = 0; x < static_cast<int>(width); ++x) {
            const float u = (static_cast<float>(x) + 0.5f) / static_cast<float>(width);
            const float v = (static_cast<float>(y) + 0.5f) / static_cast<float>(height);
            const std::size_t offset = (static_cast<std::size_t>(y) * width + x) * 4;
            
            const float dx = (u - 0.5f) * aspect;
            const float dy = (v - 0.5f);
            const float dist = std::sqrt(dx * dx + dy * dy);
            
            if (dist < progress * 0.85f && to_data) {
                std::memcpy(out_data + offset, to_data + offset, 4 * sizeof(float));
            } else {
                std::memcpy(out_data + offset, from_data + offset, 4 * sizeof(float));
            }
        }
    }
}

void apply_flash_cut_fused_direct(
    SurfaceRGBA& output,
    const SurfaceRGBA& from,
    const SurfaceRGBA* to,
    float progress,
    int thread_count) {

    const uint32_t width = output.width();
    const uint32_t height = output.height();
    const float* from_data = from.pixels().data();
    const float* to_data = to ? to->pixels().data() : nullptr;
    float* out_data = output.mutable_pixels().data();

    const float flash = 1.0f - std::abs(progress - 0.5f) * 2.0f;
    const float flash_amount = smoothstep01(std::max(0.0f, flash)) * 0.9f;
    const float eased_progress = smoothstep01(progress);

    #pragma omp parallel for num_threads(thread_count) schedule(static)
    for (int y = 0; y < static_cast<int>(height); ++y) {
        for (int x = 0; x < static_cast<int>(width); ++x) {
            const std::size_t offset = (static_cast<std::size_t>(y) * width + x) * 4;
            
            const float* src = from_data + offset;
            const float* dst = to_data ? (to_data + offset) : src;
            float* out = out_data + offset;

            float r = src[0] * (1.0f - eased_progress) + dst[0] * eased_progress;
            float g = src[1] * (1.0f - eased_progress) + dst[1] * eased_progress;
            float b = src[2] * (1.0f - eased_progress) + dst[2] * eased_progress;
            float a = src[3] * (1.0f - eased_progress) + dst[3] * eased_progress;

            out[0] = r * (1.0f - flash_amount) + flash_amount;
            out[1] = g * (1.0f - flash_amount) + flash_amount;
            out[2] = b * (1.0f - flash_amount) + flash_amount;
            out[3] = a;
        }
    }
}

} // namespace tachyon::renderer2d
