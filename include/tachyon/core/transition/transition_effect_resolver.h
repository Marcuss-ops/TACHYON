#pragma once

#include "tachyon/core/transition/transition_descriptor.h"
#include "tachyon/renderer2d/core/framebuffer.h"
#include <string>
#include <functional>

namespace tachyon {

/**
 * @brief Policy for handling fallback when transition resolution fails.
 */
enum class TransitionFallbackPolicy {
    Strict,      ///< Fail hard - no fallback
    Magenta,     ///< Return magenta diagnostic surface
    NoOp         ///< Fallback to no-op (passthrough)
};

/**
 * @brief Request structure for resolving a transition effect.
 */
struct TransitionEffectRequest {
    std::string transition_id;
    TransitionRuntimeKind preferred_backend{TransitionRuntimeKind::CpuPixel};
    TransitionFallbackPolicy fallback_policy{TransitionFallbackPolicy::Magenta};
};

/**
 * @brief Transition kernel function signature.
 * Takes input surfaces and progress (0.0 to 1.0) and returns output surface.
 */
using TransitionKernelFn = std::function<renderer2d::SurfaceRGBA(const renderer2d::SurfaceRGBA&, const renderer2d::SurfaceRGBA*, float)>;

/**
 * @brief Kernel wrapper for executing a resolved transition.
 */
struct TransitionKernel {
    TransitionKernelFn apply;
    bool valid{false};
};

/**
 * @brief Diagnostics information for transition resolution.
 */
struct TransitionDiagnostics {
    std::string error_message;
    std::string transition_id;
    bool has_error{false};
};

/**
 * @brief Resolved transition effect ready for execution.
 */
struct ResolvedTransitionEffect {
    const TransitionDescriptor* descriptor{nullptr};
    TransitionKernel kernel;
    TransitionDiagnostics diagnostics;
    bool valid{false};
};

/**
 * @brief Resolver/executor intermediate layer for transition effects.
 * 
 * This class decouples GlslTransitionEffect from resolution logic,
 * policy handling, fallback decisions, and diagnostics.
 */
class TransitionEffectResolver {
public:
    explicit TransitionEffectResolver(const TransitionRegistry& registry) : registry_(registry) {}
    
    /**
     * @brief Resolves a transition effect request into a ready-to-use effect.
     * 
     * @param request The transition effect request with ID, backend, and policy.
     * @return ResolvedTransitionEffect with descriptor, kernel, and diagnostics.
     */
    ResolvedTransitionEffect resolve(const TransitionEffectRequest& request) const;
    
private:
    const TransitionRegistry& registry_;
};

} // namespace tachyon
