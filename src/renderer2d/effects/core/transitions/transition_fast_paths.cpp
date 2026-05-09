#include "tachyon/renderer2d/effects/core/transitions/transition_fast_paths.h"
#include "tachyon/core/ids/builtin_ids.h"
#include "tachyon/core/transition/transition_simd_kernels.h"
#include <algorithm>
#include <cstring>

#ifdef _OPENMP
#include <omp.h>
#endif

namespace tachyon::renderer2d {

namespace {

void apply_fade_to_black_direct(
    float* out, 
    const float* from, 
    const float* to, 
    float t, 
    size_t count, 
    int thread_count) {
    
    const int omp_threads = thread_count > 0 ? thread_count : 1;
    
    if (t < 0.5f) {
        // Fade from 'from' to black
        const float tt = t * 2.0f;
        const float inv_tt = 1.0f - tt;
        
#ifdef _OPENMP
        #pragma omp parallel for schedule(static) num_threads(omp_threads)
#endif
        for (int i = 0; i < static_cast<int>(count); i += 4) {
            out[i]     = from[i]     * inv_tt;
            out[i + 1] = from[i + 1] * inv_tt;
            out[i + 2] = from[i + 2] * inv_tt;
            out[i + 3] = from[i + 3] * inv_tt + tt;
        }
    } else {
        // Fade from black to 'to'
        const float tt = (t - 0.5f) * 2.0f;
        const float inv_tt = 1.0f - tt;
        
#ifdef _OPENMP
        #pragma omp parallel for schedule(static) num_threads(omp_threads)
#endif
        for (int i = 0; i < static_cast<int>(count); i += 4) {
            if (to) {
                out[i]     = to[i]     * tt;
                out[i + 1] = to[i + 1] * tt;
                out[i + 2] = to[i + 2] * tt;
                out[i + 3] = to[i + 3] * tt + inv_tt;
            } else {
                out[i]     = from[i]     * tt;
                out[i + 1] = from[i + 1] * tt;
                out[i + 2] = from[i + 2] * tt;
                out[i + 3] = from[i + 3] * tt + inv_tt;
            }
        }
    }
}

void apply_wipe_linear_direct(
    float* out,
    const float* from,
    const float* to,
    uint32_t width,
    uint32_t height,
    float t,
    int thread_count) {
    
    const int omp_threads = thread_count > 0 ? thread_count : 1;
    const uint32_t split_x = static_cast<uint32_t>(static_cast<float>(width) * t);
    const size_t stride = static_cast<size_t>(width) * 4;
    
#ifdef _OPENMP
    #pragma omp parallel for schedule(static) num_threads(omp_threads)
#endif
    for (int y = 0; y < static_cast<int>(height); ++y) {
        const size_t row_offset = static_cast<size_t>(y) * stride;
        
        // Left part (target)
        if (split_x > 0) {
            if (to) {
                std::memcpy(out + row_offset, to + row_offset, split_x * 4 * sizeof(float));
            } else {
                std::memcpy(out + row_offset, from + row_offset, split_x * 4 * sizeof(float));
            }
        }
        
        // Right part (source)
        if (split_x < width) {
            const size_t remaining = width - split_x;
            std::memcpy(out + row_offset + split_x * 4, from + row_offset + split_x * 4, remaining * 4 * sizeof(float));
        }
    }
}

} // namespace

bool apply_transition_fast_path(
    const std::string& transition_id,
    SurfaceRGBA& output,
    const SurfaceRGBA& from,
    const SurfaceRGBA* to,
    float progress,
    int thread_count) {
    
    const uint32_t w = from.width();
    const uint32_t h = from.height();
    
    // Fast paths require dimension matching
    if (output.width() != w || output.height() != h) return false;
    if (to && (to->width() != w || to->height() != h)) return false;
    
    const size_t pixel_count = static_cast<size_t>(w) * h * 4;
    const float t = std::clamp(progress, 0.0f, 1.0f);
    
    if (transition_id == ids::transition::crossfade) {
        const float* from_ptr = from.pixels().data();
        const float* to_ptr = to ? to->pixels().data() : from_ptr;
        
        tachyon::runtime::simd::lerp_pixels_best(output.mutable_pixels().data(), from_ptr, to_ptr, pixel_count, t);
        return true;
    }
    
    if (transition_id == ids::transition::fade_to_black) {
        apply_fade_to_black_direct(
            output.mutable_pixels().data(), 
            from.pixels().data(), 
            to ? to->pixels().data() : nullptr, 
            t, 
            pixel_count, 
            thread_count);
        return true;
    }
    
    if (transition_id == ids::transition::wipe_linear) {
        apply_wipe_linear_direct(
            output.mutable_pixels().data(), 
            from.pixels().data(), 
            to ? to->pixels().data() : nullptr, 
            w, h, t, 
            thread_count);
        return true;
    }
    
    return false;
}

} // namespace tachyon::renderer2d
