#pragma once

#include "tachyon/core/registry/parameter_schema.h"
#include "tachyon/core/spec/schema/common/common_spec.h"
#include <string>
#include <vector>
#include <optional>
#include <functional>
#include <unordered_map>

namespace tachyon {

class TransitionRegistry;

namespace renderer2d {
struct Color;
class SurfaceRGBA;
} // namespace renderer2d

// Alias TransitionCategory to existing TransitionKind for logical categorization
using TransitionCategory = TransitionKind;

/**
 * @brief Canonical runtime kind (backend type) for transitions.
 * Matches former TransitionBackend.
 */
enum class TransitionRuntimeKind {
    StateOnly,    ///< Affects only layer properties (opacity, transform)
    CpuPixel,     ///< Pixel-level manipulation on CPU (SurfaceRGBA)
    Glsl,         ///< GPU-accelerated shader transition
    CompositePlan ///< Multi-layer complex orchestration
};

/**
 * @brief CPU transition function signature (matches legacy TransitionSpec::TransitionFn)
 */
using CpuTransitionFn = renderer2d::Color(*)(float u, float v, float t,
                                              const renderer2d::SurfaceRGBA& input,
                                              const renderer2d::SurfaceRGBA* to_surface);

/**
 * @brief GLSL transition function, returns shader source string
 */
using GlslTransitionFn = std::string(*)(const std::unordered_map<std::string, registry::ParameterValue>& params);

/**
 * @brief Capabilities of a transition (which backends are supported)
 */
struct TransitionCapabilities {
    bool supports_cpu{false};
    bool supports_gpu{false};
};

/**
 * @brief Detailed result of a transition resolution attempt.
 */
struct TransitionResolutionResult {
    enum class Status {
        Ok,
        UnknownTransition,   ///< ID not found in registry
        BackendMismatch,     ///< Requested backend (e.g. GPU) not supported by transition
        ParameterError,      ///< Provided parameters don't match schema
        InternalError        ///< Implementation failed (e.g. shader compile)
    };

    Status status{Status::Ok};
    std::string error_message;
    
    // Resolved implementation
    CpuTransitionFn cpu_function{nullptr};   ///< For CPU transitions
    GlslTransitionFn glsl_function{nullptr}; ///< For GLSL transitions
    TransitionRuntimeKind backend{TransitionRuntimeKind::StateOnly};
};

/**
 * @brief Unified descriptor for a Tachyon transition (SINGLE SOURCE OF TRUTH)
 */
struct TransitionDescriptor {
    std::string id;                  ///< Canonical ID (e.g. "tachyon.transition.crossfade")
    std::vector<std::string> aliases; ///< Alternative IDs/aliases for this transition

    TransitionRuntimeKind runtime_kind; ///< Backend type
    TransitionCategory category;       ///< Logical category (Fade, Slide, etc.)

    registry::ParameterSchema params;   ///< Parameter validation schema (ParamSchema)
    TransitionCapabilities capabilities; ///< Supported backends

    CpuTransitionFn cpu_fn{nullptr};   ///< CPU implementation function
    GlslTransitionFn glsl_fn{nullptr}; ///< GLSL shader function

    std::string display_name;          ///< UI display name
    std::string description;           ///< Human-readable description
};

/**
 * @brief Catalog entry derived from TransitionDescriptor (read-only view for UI/catalog)
 */
struct TransitionCatalogEntry {
    std::string id;
    std::string display_name;
    std::string description;
    TransitionCategory category;
    std::vector<std::string> aliases;
    bool supports_cpu;
    bool supports_gpu;
};

/**
 * @brief Resolves a transition ID into a detailed result, respecting engine policy.
 */
TACHYON_API TransitionResolutionResult resolve_transition(const std::string& id, const TransitionRegistry& registry);

/**
 * @brief Registers all built-in transitions.
 */
TACHYON_API void register_builtin_transitions(TransitionRegistry& registry);

} // namespace tachyon
