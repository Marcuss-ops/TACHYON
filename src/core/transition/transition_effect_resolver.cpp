#include "tachyon/core/transition/transition_effect_resolver.h"
#include "tachyon/transition_registry.h"
#include "tachyon/core/policy/engine_policy.h"
#include "tachyon/renderer2d/core/framebuffer.h"
#include "tachyon/renderer2d/effects/core/transitions/transition_utils.h"
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
int resolve_openmp_threads() {
#ifdef _OPENMP
    int omp_threads = 1;
    const char* omp_threads_env = std::getenv("TACHYON_OPENMP_THREADS_PER_FRAME");
    if (omp_threads_env) {
        char* endptr = nullptr;
        long val = std::strtol(omp_threads_env, &endptr, 10);
        if (endptr != omp_threads_env && val > 0) {
            omp_threads = static_cast<int>(val);
        }
    } else {
        unsigned int hw = std::thread::hardware_concurrency();
        if (hw == 0) {
            hw = 1;
        }
        omp_threads = static_cast<int>(hw);
    }

    const char* max_total_env = std::getenv("TACHYON_MAX_TOTAL_THREADS");
    if (max_total_env) {
        char* endptr = nullptr;
        long max_total = std::strtol(max_total_env, &endptr, 10);
        if (endptr != max_total_env && max_total > 0) {
            omp_threads = std::max(1, static_cast<int>(std::min<long>(max_total, omp_threads)));
        }
    }

    return std::max(1, omp_threads);
#else
    return 1;
#endif
}

// Helper to create a "none" kernel that performs simple lerp
TransitionKernel create_none_kernel() {
    TransitionKernel kernel;
    kernel.apply = [](const SurfaceRGBA& input_a, const SurfaceRGBA* input_b, float progress) {
        SurfaceRGBA output;
        output.reset(input_a.width(), input_a.height());
        output.set_profile(input_a.profile());
        auto& pixels = output.mutable_pixels();

        const float width = static_cast<float>(std::max<std::uint32_t>(1U, input_a.width()));
        const float height = static_cast<float>(std::max<std::uint32_t>(1U, input_a.height()));

        const std::size_t width_u = static_cast<std::size_t>(input_a.width());
        const int height_i = static_cast<int>(input_a.height());
        const int width_i = static_cast<int>(input_a.width());
        [[maybe_unused]] const int omp_threads = resolve_openmp_threads();

#ifdef _OPENMP
        #pragma omp parallel for schedule(static) num_threads(omp_threads)
#endif
        for (int y = 0; y < height_i; ++y) {
            for (int x = 0; x < width_i; ++x) {
                const float u = (static_cast<float>(x) + 0.5f) / width;
                const float v = (static_cast<float>(y) + 0.5f) / height;
                Color out = lerp_surface_color(input_a, input_b, u, v, progress);
                const std::size_t index = (static_cast<std::size_t>(y) * width_u + static_cast<std::size_t>(x)) * 4U;
                pixels[index] = out.r;
                pixels[index + 1] = out.g;
                pixels[index + 2] = out.b;
                pixels[index + 3] = out.a;
            }
        }
        return output;
    };
    kernel.valid = true;
    return kernel;
}

// Helper to create a kernel from a CPU transition function
TransitionKernel create_cpu_kernel(CpuTransitionFn cpu_fn) {
    if (!cpu_fn) {
        return TransitionKernel{};
    }

    TransitionKernel kernel;
    kernel.apply = [cpu_fn](const SurfaceRGBA& input_a, const SurfaceRGBA* input_b, float progress) {
        SurfaceRGBA output;
        output.reset(input_a.width(), input_a.height());
        output.set_profile(input_a.profile());
        auto& pixels = output.mutable_pixels();

        const float width = static_cast<float>(std::max<std::uint32_t>(1U, input_a.width()));
        const float height = static_cast<float>(std::max<std::uint32_t>(1U, input_a.height()));

        const std::size_t width_u = static_cast<std::size_t>(input_a.width());
        const int height_i = static_cast<int>(input_a.height());
        const int width_i = static_cast<int>(input_a.width());
        [[maybe_unused]] const int omp_threads = resolve_openmp_threads();

#ifdef _OPENMP
        #pragma omp parallel for schedule(static) num_threads(omp_threads)
#endif
        for (int y = 0; y < height_i; ++y) {
            for (int x = 0; x < width_i; ++x) {
                const float u = (static_cast<float>(x) + 0.5f) / width;
                const float v = (static_cast<float>(y) + 0.5f) / height;
                Color out = cpu_fn(u, v, progress, input_a, input_b);
                const std::size_t index = (static_cast<std::size_t>(y) * width_u + static_cast<std::size_t>(x)) * 4U;
                pixels[index] = out.r;
                pixels[index + 1] = out.g;
                pixels[index + 2] = out.b;
                pixels[index + 3] = out.a;
            }
        }
        return output;
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
        result.kernel = create_cpu_kernel(desc->cpu_fn);
        result.valid = result.kernel.valid;
    } else {
        // No CPU function available - this is an error for CpuPixel backend
        result.diagnostics.has_error = true;
        result.diagnostics.error_message = "Transition '" + desc->id + "' has no CPU implementation.";
    }

    return result;
}

} // namespace tachyon
