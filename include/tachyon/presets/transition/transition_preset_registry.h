#pragma once

#include "tachyon/presets/transition/transition_params.h"
#include "tachyon/core/spec/schema/objects/layer_spec.h"
#include <string>
#include <string_view>
#include <functional>
#include <memory>
#include <vector>
#include <optional>

namespace tachyon::presets {

/**
 * @brief Specification for a transition preset.
 */
struct TransitionPresetSpec {
    std::string id;
    std::string name;
    std::string description;
    
    // Factory function to create the transition spec based on parameters.
    std::function<LayerTransitionSpec(const TransitionParams&)> factory;
};

/**
 * @brief Registry for transition presets.
 * Maps transition IDs to factory functions that create LayerTransitionSpec.
 */
class TransitionPresetRegistry {
public:
    static TransitionPresetRegistry& instance();

    // Registers a new transition preset.
    void register_preset(TransitionPresetSpec spec);

    // Finds a preset by ID.
    const TransitionPresetSpec* find(std::string_view id) const;

    // Creates an enter transition using the specified ID and parameters.
    LayerTransitionSpec create_enter(const TransitionParams& params) const;

    // Creates an exit transition using the specified ID and parameters.
    LayerTransitionSpec create_exit(const TransitionParams& params) const;

    // Lists all registered transition IDs.
    std::vector<std::string> list_ids() const;

    // Loads built-in presets from the table.
    void load_builtins();

private:
    TransitionPresetRegistry();
    ~TransitionPresetRegistry() = default;

    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace tachyon::presets
