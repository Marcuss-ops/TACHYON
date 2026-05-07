#include "tachyon/core/transition/transition_effect_resolver.h"
#include "tachyon/transition_registry.h"
#include "tachyon/core/policy/engine_policy.h"
#include "tachyon/renderer2d/raster/surface.h"
#include "tachyon/renderer2d/color/color.h"
#include "tachyon/renderer2d/effects/core/transitions/transition_utils.h"
#include <iostream>

namespace tachyon {

namespace {
// Helper to create a "none" kernel that performs simple lerp
TransitionKernel create_none_kernel() {
    TransitionKernel kernel;
    kernel.apply = [](const SurfaceRGBA& input_a, const SurfaceRGBA* input_b, float progress) {
        SurfaceRGBA output(input_a.width(), input_a.height());
        output.set_profile(input_a.profile());

        const float width = static_cast<float>(std::max<std::uint32_t>(1U, input_a.width()));
        const float height = static_cast<float>(std::max<std::uint32_t>(1U, input_a.height()));

        for (std::uint32_t y = 0; y < input_a.height(); ++y) {
            for (std::uint32_t x = 0; x < input_a.width(); ++x) {
                const float u = (static_cast<float>(x) + 0.5f) / width;
                const float v = (static_cast<float>(y) + 0.5f) / height;
                Color out = lerp_surface_color(input_a, input_b, u, v, progress);
                output.set_pixel(x, y, out);
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
        SurfaceRGBA output(input_a.width(), input_a.height());
        output.set_profile(input_a.profile());

        const float width = static_cast<float>(std::max<std::uint32_t>(1U, input_a.width()));
        const float height = static_cast<float>(std::max<std::uint32_t>(1U, input_a.height()));

        for (std::uint32_t y = 0; y < input_a.height(); ++y) {
            for (std::uint32_t x = 0; x < input_a.width(); ++x) {
                const float u = (static_cast<float>(x) + 0.5f) / width;
                const float v = (static_cast<float>(y) + 0.5f) / height;
                Color out = cpu_fn(u, v, progress, input_a, input_b);
                output.set_pixel(x, y, out);
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
    const auto* desc = TransitionRegistry::instance().resolve(request.transition_id);

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
