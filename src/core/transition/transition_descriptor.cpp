#include "tachyon/core/transition/transition_descriptor.h"
#include "tachyon/transition_registry.h"
#include "tachyon/presets/transition/transition_preset_registry.h"
#include "tachyon/core/registry/parameter_bag.h"
#include "tachyon/core/registry/registry_metadata.h"
#include "tachyon/core/policy/engine_policy.h"

namespace tachyon {

TransitionDescriptorRegistry& TransitionDescriptorRegistry::instance() {
    static TransitionDescriptorRegistry registry;
    return registry;
}

void TransitionDescriptorRegistry::register_descriptor(const TransitionDescriptor& desc) {
    m_descriptors[desc.id] = desc;
}

const TransitionDescriptor* TransitionDescriptorRegistry::find(const std::string& id) const {
    auto it = m_descriptors.find(id);
    if (it != m_descriptors.end()) {
        return &it->second;
    }
    return nullptr;
}

std::vector<const TransitionDescriptor*> TransitionDescriptorRegistry::list_all() const {
    std::vector<const TransitionDescriptor*> result;
    for (auto& [id, desc] : m_descriptors) {
        result.push_back(&desc);
    }
    return result;
}

TransitionResolutionResult resolve_transition(const std::string& id) {
    TransitionResolutionResult result;
    
    if (id.empty() || id == "none" || id == "tachyon.transition.none") {
        result.status = TransitionResolutionResult::Status::Ok;
        result.backend = TransitionBackend::StateOnly;
        result.renderer_effect_id = "none";
        return result;
    }

    const auto* desc = TransitionDescriptorRegistry::instance().find(id);
    if (!desc) {
        if (EngineValidationPolicy::instance().strict_transitions) {
            result.status = TransitionResolutionResult::Status::UnknownTransition;
            result.error_message = "Transition '" + id + "' is not registered in the canonical registry.";
        } else {
            // Fallback to no-op if not strict
            result.status = TransitionResolutionResult::Status::Ok;
            result.backend = TransitionBackend::StateOnly;
            result.renderer_effect_id = "none";
        }
        return result;
    }

    // Success
    result.status = TransitionResolutionResult::Status::Ok;
    result.backend = desc->backend;
    result.renderer_effect_id = desc->renderer_effect_id;
    
    // Resolve implementation function
    if (desc->runtime_fn) {
        result.function = desc->runtime_fn;
    } else if (!desc->cpu_fn_name.empty()) {
        result.function = TransitionRegistry::instance().find_cpu_implementation(desc->cpu_fn_name);
    }

    return result;
}

void register_transition_descriptor(const TransitionDescriptor& desc) {
    // 0. Register in Descriptor Registry (Source of Truth)
    TransitionDescriptorRegistry::instance().register_descriptor(desc);

    // 1. Register in Runtime TransitionRegistry
    TransitionSpec runtime_spec;
    runtime_spec.id = desc.id;
    runtime_spec.name = desc.display_name;
    runtime_spec.description = desc.description;
    runtime_spec.cpu_fn_name = desc.cpu_fn_name;
    runtime_spec.renderer_effect_id = desc.renderer_effect_id;
    
    // Resolve implementation
    if (desc.runtime_fn) {
        runtime_spec.function = desc.runtime_fn;
    } else if (!desc.cpu_fn_name.empty()) {
        runtime_spec.function = TransitionRegistry::instance().find_cpu_implementation(desc.cpu_fn_name);
    }
    
    // Map TransitionKind to TransitionSpec::Type (temporary alignment)
    switch (desc.kind) {
        case TransitionKind::Fade:  runtime_spec.state_type = TransitionSpec::Type::Fade; break;
        case TransitionKind::Slide: runtime_spec.state_type = TransitionSpec::Type::Slide; break;
        case TransitionKind::Zoom:  runtime_spec.state_type = TransitionSpec::Type::Zoom; break;
        case TransitionKind::Flip:  runtime_spec.state_type = TransitionSpec::Type::Flip; break;
        default:                    runtime_spec.state_type = TransitionSpec::Type::None; break;
    }
    
    switch (desc.backend) {
        case TransitionBackend::CpuPixel:     runtime_spec.backend = TransitionSpec::Backend::CpuPixel; break;
        case TransitionBackend::Glsl:         runtime_spec.backend = TransitionSpec::Backend::Glsl; break;
        case TransitionBackend::StateOnly:    runtime_spec.backend = TransitionSpec::Backend::StateOnly; break;
        case TransitionBackend::CompositePlan: runtime_spec.backend = TransitionSpec::Backend::CompositePlan; break;
    }
    
    TransitionRegistry::instance().register_transition_from_descriptor(runtime_spec);
    
    // 2. Register in TransitionPresetRegistry
    presets::TransitionPresetSpec preset_spec;
    preset_spec.id = desc.id;
    preset_spec.metadata.id = desc.id;
    preset_spec.metadata.display_name = desc.display_name;
    preset_spec.metadata.description = desc.description;
    preset_spec.metadata.category = desc.category;
    preset_spec.metadata.tags = desc.tags;
    preset_spec.metadata.stability = registry::Stability::Stable;
    
    // Schema alignment
    preset_spec.schema = desc.schema;
    
    // Factory that just creates a LayerTransitionSpec pointing to this ID
    preset_spec.factory = [id = desc.id, duration = desc.default_duration](const registry::ParameterBag& params) {
        LayerTransitionSpec spec;
        spec.transition_id = id;
        spec.duration = params.get_or<double>("duration", duration);
        // Copy other standard parameters if needed
        return spec;
    };
    
    presets::TransitionPresetRegistry::instance().register_spec(std::move(preset_spec));
}

} // namespace tachyon
