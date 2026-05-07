#pragma once

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "tachyon/core/registry/typed_registry.h"

namespace tachyon {

namespace renderer2d {
struct Color;
class SurfaceRGBA;
} // namespace renderer2d

/**
 * TransitionSpec: pure specification for a transition.
 * Supports both state-level and pixel-level transitions.
 */
struct TransitionSpec {
    // Pixel-level transition function (for surface blending)
    using TransitionFn = renderer2d::Color(*)(float u, float v, float t,
                                              const renderer2d::SurfaceRGBA& input,
                                              const renderer2d::SurfaceRGBA* to_surface);

    std::string id;
    std::string name;
    std::string description;

    // Pixel-level transition (blends surfaces)
    TransitionFn function{nullptr};

    // State-level transition info (modifies layer properties)
    enum class Type {
        None,
        Fade,   // Modify opacity
        Slide,  // Modify position
        Zoom,   // Modify scale
        Flip,   // Modify rotation/scale
        Blur    // Modify effects
    };
    Type state_type{Type::None};
    // Backend declaration
    enum class Backend {
        CpuPixel,
        Glsl,
        StateOnly,
        CompositePlan
    };
    Backend backend{Backend::CpuPixel};

    std::string direction; // For slide: "up", "down", "left", "right"
    std::string cpu_fn_name; // For lazy binding
    std::string renderer_effect_id; // For GPU/Shader binding
};

/**
 * Runtime registry for transitions.
 * Supports both state-level and pixel-level transitions.
 */
class TransitionRegistry {
public:
    static TransitionRegistry& instance();

    void register_transition(const TransitionSpec& spec);
    void unregister_transition(const std::string& id);

    const TransitionSpec* find(const std::string& id) const;
    std::size_t count() const;
    const TransitionSpec* get_by_index(std::size_t index) const;

    struct TransitionInfo {
        std::string id;
        std::string name;
        std::string description;
        bool has_pixel_function;
        bool has_state_type;
    };

    std::vector<std::string> list_builtin_transition_ids() const;
    std::vector<TransitionInfo> list_builtin_transitions() const;

    // Named CPU implementation support (for lazy manifest binding)
    void register_cpu_implementation(const std::string& name, TransitionSpec::TransitionFn fn);
    TransitionSpec::TransitionFn find_cpu_implementation(const std::string& name) const;

private:
    TransitionRegistry();
    ~TransitionRegistry();
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace tachyon
