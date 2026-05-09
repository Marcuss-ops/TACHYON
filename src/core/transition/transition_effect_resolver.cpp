#include "tachyon/core/transition/transition_effect_resolver.h"
#include "tachyon/transition_registry.h"
#include "tachyon/core/policy/engine_policy.h"
#include "tachyon/renderer2d/core/framebuffer.h"
#include "tachyon/renderer2d/effects/core/transitions/transition_utils.h"
#include "tachyon/renderer2d/effects/core/transitions/transition_fast_paths.h"
#include "tachyon/core/transition/transition_simd_kernels.h"
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <thread>

#ifdef _OPENMP
#include <omp.h>
#endif

namespace tachyon {
using namespace renderer2d;

using renderer2d::Color;
using renderer2d::SurfaceRGBA;

namespace {

// Helper to create a "none" kernel that performs simple lerp
TransitionKernel create_none_kernel() {
    TransitionKernel kernel;
    kernel.apply = [](SurfaceRGBA& output, const SurfaceRGBA& input_a, const SurfaceRGBA* input_b, float progress, int thread_count) {
        const std::uint32_t width_u32 = input_a.width();
        const std::uint32_t height_u32 = input_a.height();
        
        // Ensure output is the right size
        if (output.width() != width_u32 || output.height() != height_u32) {
            output.reset(width_u32, height_u32);
        }
        output.set_profile(input_a.profile());
        auto& pixels = output.mutable_pixels();

        const int omp_threads = thread_count > 0 ? thread_count : 1;

        // Fast path: dimensions match exactly
        if (input_b && input_b->width() == width_u32 && input_b->height() == height_u32) {
            const float t = std::clamp(progress, 0.0f, 1.0f);
            
#ifdef _OPENMP
            #pragma omp parallel for schedule(static) num_threads(omp_threads)
#endif
            for (int y = 0; y < static_cast<int>(height_u32); ++y) {
                const std::size_t offset = static_cast<std::size_t>(y) * width_u32 * 4;
                tachyon::runtime::simd::lerp_pixels_best(
                    pixels.data() + offset, 
                    input_a.pixels().data() + offset, 
                    input_b->pixels().data() + offset, 
                    static_cast<std::size_t>(width_u32) * 4, 
                    t);
            }
            return;
        }

        // Fallback for mismatched sizes
        const float width_f = static_cast<float>(std::max<std::uint32_t>(1U, width_u32));
        const float height_f = static_cast<float>(std::max<std::uint32_t>(1U, height_u32));
        const std::size_t stride = static_cast<std::size_t>(width_u32) * 4;

#ifdef _OPENMP
        #pragma omp parallel for schedule(static) num_threads(omp_threads)
#endif
        for (int y = 0; y < static_cast<int>(height_u32); ++y) {
            const std::size_t row_start = static_cast<std::size_t>(y) * stride;
            for (int x = 0; x < static_cast<int>(width_u32); ++x) {
                const float u = (static_cast<float>(x) + 0.5f) / width_f;
                const float v = (static_cast<float>(y) + 0.5f) / height_f;
                Color out = lerp_surface_color(input_a, input_b, u, v, progress);
                
                const std::size_t index = row_start + (static_cast<std::size_t>(x) * 4);
                pixels[index] = out.r;
                pixels[index + 1] = out.g;
                pixels[index + 2] = out.b;
                pixels[index + 3] = out.a;
            }
        }
    };
    kernel.valid = true;
    return kernel;
}

// Helper to create a kernel from a CPU transition function
TransitionKernel create_cpu_kernel(const std::string& transition_id, CpuTransitionFn cpu_fn) {
    if (!cpu_fn) {
        return TransitionKernel{};
    }

    TransitionKernel kernel;
    kernel.apply = [transition_id, cpu_fn](SurfaceRGBA& output, const SurfaceRGBA& input_a, const SurfaceRGBA* input_b, float progress, int thread_count) {
        // Attempt fast path first
        if (renderer2d::apply_transition_fast_path(transition_id, output, input_a, input_b, progress, thread_count)) {
            return;
        }

        const std::uint32_t width_u32 = input_a.width();
        const std::uint32_t height_u32 = input_a.height();
        
        // Ensure output is the right size
        if (output.width() != width_u32 || output.height() != height_u32) {
            output.reset(width_u32, height_u32);
        }
        output.set_profile(input_a.profile());
        auto& pixels = output.mutable_pixels();

        const float width_f = static_cast<float>(std::max<std::uint32_t>(1U, width_u32));
        const float height_f = static_cast<float>(std::max<std::uint32_t>(1U, height_u32));
        const std::size_t stride = static_cast<std::size_t>(width_u32) * 4;
        const int omp_threads = thread_count > 0 ? thread_count : 1;

#ifdef _OPENMP
        #pragma omp parallel for schedule(static) num_threads(omp_threads)
#endif
        for (int y = 0; y < static_cast<int>(height_u32); ++y) {
            const std::size_t row_start = static_cast<std::size_t>(y) * stride;
            for (int x = 0; x < static_cast<int>(width_u32); ++x) {
                const float u = (static_cast<float>(x) + 0.5f) / width_f;
                const float v = (static_cast<float>(y) + 0.5f) / height_f;
                Color out = cpu_fn(u, v, progress, input_a, input_b);
                
                const std::size_t index = row_start + (static_cast<std::size_t>(x) * 4);
                pixels[index] = out.r;
                pixels[index + 1] = out.g;
                pixels[index + 2] = out.b;
                pixels[index + 3] = out.a;
            }
        }
    };
    kernel.valid = true;
    return kernel;
}
} // anonymous namespace

ResolvedTransitionEffect TransitionEffectResolver::resolve(const TransitionEffectRequest& request) const {
    ResolvedTransitionEffect result;

    // Handle empty transition_id or "none" - these are valid "no transition" cases
    if (request.transition_id.empty() || request.transition_id == "none" ||
        request.transition_id == "tachyon.transition.none") {
        result.kernel = create_none_kernel();
        result.valid = true;
        return result;
    }

    // Look up in TransitionRegistry (canonical source of truth)
    const auto* desc = m_registry.resolve(request.transition_id);

    if (!desc) {
        // Transition not found - this is an error
        result.diagnostics.has_error = true;
        result.diagnostics.transition_id = request.transition_id;
        result.diagnostics.error_message = "Transition '" + request.transition_id + "' is not registered in the canonical registry.";
        return result;
    }

    result.descriptor = desc;
    result.diagnostics.transition_id = desc->id;

    // Check backend compatibility
    if (desc->runtime_kind != request.preferred_backend &&
        request.preferred_backend != TransitionRuntimeKind::StateOnly) {
        result.diagnostics.has_error = true;
        result.diagnostics.error_message = "Transition '" + desc->id + "' does not support requested backend.";
        return result;
    }

    // Create kernel function from the descriptor's CPU function
    if (desc->cpu_fn != nullptr) {
        result.kernel = create_cpu_kernel(desc->id, desc->cpu_fn);
        result.valid = result.kernel.valid;
    } else {
        // No CPU function available - this is an error for CpuPixel backend
        result.diagnostics.has_error = true;
        result.diagnostics.error_message = "Transition '" + desc->id + "' has no CPU implementation.";
    }

    return result;
}

} // namespace tachyon
