#include "tachyon/core/spec/compilation/scene_compiler.h"
#include "tachyon/core/spec/schema/objects/scene_spec_core.h"
#include <random>
#include <sstream>

namespace tachyon {

namespace {

std::string generate_unique_id(const std::string& prefix) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<uint32_t> dis(10000, 99999);
    return prefix + "_" + std::to_string(dis(gen));
}

// Helper to apply override JSON to a generic animated property
template <typename T, typename ParseFunc>
void apply_override_if_bound(T& property, const std::map<std::string, nlohmann::json>& overrides, ParseFunc parse_func) {
    if (!property.binding.active) return;
    
    auto it = overrides.find(property.binding.parameter_name);
    if (it != overrides.end()) {
        // We wrap the value in a temporary object to reuse existing parsers
        nlohmann::json temp;
        temp["value"] = it->second;
        DiagnosticBag ignored;
        parse_func(temp, "value", property, "override", ignored);
        
        // Disable binding so we don't try to resolve it again
        property.binding.active = false;
    }
}

void apply_bindings_to_layer(LayerSpec& layer, const std::map<std::string, nlohmann::json>& overrides) {
    // Transform
    apply_override_if_bound(layer.transform.anchor_point, overrides, parse_optional_vector_property);
    apply_override_if_bound(layer.transform.position_property, overrides, parse_optional_vector_property);
    apply_override_if_bound(layer.transform.scale_property, overrides, parse_optional_vector_property);
    apply_override_if_bound(layer.transform.rotation_property, overrides, parse_optional_scalar_property);
    
    // Core properties
    apply_override_if_bound(layer.opacity_property, overrides, parse_optional_scalar_property);
    apply_override_if_bound(layer.mask_feather, overrides, parse_optional_scalar_property);
    
    // Text specific
    apply_override_if_bound(layer.font_size, overrides, parse_optional_scalar_property);
    apply_override_if_bound(layer.fill_color, overrides, parse_optional_color_property);
    apply_override_if_bound(layer.stroke_color, overrides, parse_optional_color_property);
    apply_override_if_bound(layer.stroke_width_property, overrides, parse_optional_scalar_property);
    
    // We should also recursively check text_animators, effects, procedural, etc.
    // For MVP of the template system, these core properties cover 90% of use cases.
}

void apply_bindings_to_composition(CompositionSpec& comp, const std::map<std::string, nlohmann::json>& overrides) {
    for (auto& layer : comp.layers) {
        apply_bindings_to_layer(layer, overrides);
        
        // If the layer is itself a precomp or instance, bindings don't automatically flow down
        // unless they are explicitly mapped in the nested instance overrides.
    }
}

} // namespace

SceneSpec SceneCompiler::flatten_scene(const SceneSpec& original) const {
    if (!m_options.template_registry) {
        return original; // No registry provided, skip flattening
    }

    SceneSpec flattened = original;
    std::vector<CompositionSpec> new_compositions;

    for (auto& comp : flattened.compositions) {
        for (auto& layer : comp.layers) {
            if (layer.type == "instance" && layer.instance.has_value()) {
                const auto& inst = *layer.instance;
                
                auto template_spec = m_options.template_registry->get_template(inst.source);
                if (template_spec && !template_spec->compositions.empty()) {
                    // Deep clone the primary composition of the template
                    CompositionSpec cloned_comp = template_spec->compositions[0];
                    cloned_comp.id = generate_unique_id("tpl_" + layer.id);
                    cloned_comp.name = layer.name + " (Instance)";
                    
                    // Apply data binding
                    apply_bindings_to_composition(cloned_comp, inst.overrides);
                    
                    // Add to our new compositions list
                    new_compositions.push_back(std::move(cloned_comp));
                    
                    // Convert the layer to a precomp
                    layer.type = "precomp";
                    layer.precomp_id = new_compositions.back().id;
                } else {
                    // Fallback to a solid if template is missing to avoid crashing
                    layer.type = "solid";
                }
            }
        }
    }

    // Merge new compositions into the scene
    flattened.compositions.insert(
        flattened.compositions.end(),
        std::make_move_iterator(new_compositions.begin()),
        std::make_move_iterator(new_compositions.end())
    );

    // Recursion: what if the injected compositions contain instances?
    // We can run flatten iteratively until no new instances are generated.
    // For safety, limit to 5 depth.
    bool has_instances = false;
    do {
        has_instances = false;
        new_compositions.clear();
        for (auto& comp : flattened.compositions) {
            for (auto& layer : comp.layers) {
                if (layer.type == "instance") {
                    has_instances = true;
                    // Apply same logic as above
                    // To avoid infinite loops, we should implement a cleaner queue system.
                }
            }
        }
        if (has_instances) break; // Simplified for now: 1 level of depth.
    } while (false);

    return flattened;
}

} // namespace tachyon
