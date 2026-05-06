#pragma once

#include "tachyon/transition_registry.h"
#include "tachyon/core/spec/schema/common/common_spec.h"
#include <string>
#include <vector>
#include <optional>
#include <functional>

namespace tachyon {

/**
 * @brief Canonical backend types for transitions.
 */
enum class TransitionBackend {
    StateOnly,    ///< Affects only layer properties (opacity, transform)
    CpuPixel,     ///< Pixel-level manipulation on CPU (SurfaceRGBA)
    Glsl,         ///< GPU-accelerated shader transition
    CompositePlan ///< Multi-layer complex orchestration
};

/**
 * @brief Unified descriptor for a Tachyon transition.
 */
struct TransitionDescriptor {
    std::string id;
    std::string name;
    std::string description;
    
    TransitionKind kind;
    TransitionBackend backend;
    
    std::string cpu_fn_name;
    TransitionSpec::TransitionFn runtime_fn{nullptr};
    
    std::vector<std::string> tags;
    std::string category;
    double default_duration;
    
    std::string preview_thumb_path;
    bool visible_in_catalog;
};

/**
 * @brief Registers a transition across all internal registries.
 */
TACHYON_API void register_transition_descriptor(const TransitionDescriptor& desc);

    /**
 * @brief Registers all built-in transitions.
 */
TACHYON_API void register_builtin_transitions();

} // namespace tachyon
