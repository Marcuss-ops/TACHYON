#pragma once

#include "tachyon/core/registry/parameter_schema.h"
#include "tachyon/core/spec/schema/common/common_spec.h"
#include "tachyon/transition_registry.h"
#include <string>
#include <vector>
#include <optional>
#include <functional>
#include <unordered_map>

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
    
    // The resolved implementation (if status == Ok)
    TransitionSpec::TransitionFn function{nullptr};
    std::string renderer_effect_id;
    TransitionBackend backend{TransitionBackend::StateOnly};
};

/**
 * @brief Unified descriptor for a Tachyon transition.
 * 
 * This is the SINGLE SOURCE OF TRUTH for a transition.
 * It links the preset (catalog), the implementation (backend), and the renderer.
 */
struct TransitionDescriptor {
    std::string id;                  ///< Canonical ID (e.g. "tachyon.transition.crossfade")
    std::string display_name;        ///< UI name
    std::string description;
    
    TransitionKind kind;             ///< Logical kind (Fade, Slide, etc.)
    TransitionBackend backend;       ///< Preferred backend
    
    std::string renderer_effect_id;  ///< ID used by the renderer (e.g. "crossfade")
    registry::ParameterSchema schema; ///< Parameter validation schema
    
    bool supports_gpu{false};
    bool supports_cpu{false};
    
    // Legacy implementation hooks (to be migrated)
    std::string cpu_fn_name;
    TransitionSpec::TransitionFn runtime_fn{nullptr};
    
    // Metadata
    std::vector<std::string> tags;
    std::string category;
    double default_duration{1.0};
    
    std::string preview_thumb_path;
    bool visible_in_catalog{true};
};

/**
 * @brief Registry for TransitionDescriptors.
 */
class TransitionDescriptorRegistry {
public:
    static TransitionDescriptorRegistry& instance();

    void register_descriptor(const TransitionDescriptor& desc);
    const TransitionDescriptor* find(const std::string& id) const;
    std::vector<const TransitionDescriptor*> list_all() const;

private:
    TransitionDescriptorRegistry() = default;
    std::unordered_map<std::string, TransitionDescriptor> m_descriptors;
};

/**
 * @brief Resolves a transition ID into a detailed result, respecting engine policy.
 */
TACHYON_API TransitionResolutionResult resolve_transition(const std::string& id);

/**
 * @brief Registers a transition across all internal registries.
 */
TACHYON_API void register_transition_descriptor(const TransitionDescriptor& desc);

    /**
 * @brief Registers all built-in transitions.
 */
TACHYON_API void register_builtin_transitions();

} // namespace tachyon
