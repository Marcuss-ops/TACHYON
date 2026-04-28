#include "tachyon/core/spec/compilation/scene_compiler_detail.h"
#include "tachyon/core/spec/schema/objects/scene_spec_core.h"
#include "src/core/spec/compilation/layer_compiler.h"
#include "tachyon/core/spec/compilation/component_library.h"
#include <algorithm>
#include <string>
#include <cstdint>

namespace tachyon::detail {

void build_compositions(const SceneSpec& scene, CompiledScene& compiled, CompilationRegistry& registry, DiagnosticBag& diagnostics) {
    compiled.compositions.reserve(scene.compositions.size());
    for (const auto& composition : scene.compositions) {
        CompiledComposition compiled_composition;
        compiled_composition.node = registry.create_node(CompiledNodeType::Composition);
        compiled.graph.add_node(compiled_composition.node.node_id);
        compiled_composition.width = static_cast<std::uint32_t>(std::max<std::int64_t>(0, composition.width));
        compiled_composition.height = static_cast<std::uint32_t>(std::max<std::int64_t>(0, composition.height));
        compiled_composition.duration = composition.duration;
        compiled_composition.fps = composition.frame_rate.numerator > 0 ? static_cast<std::uint32_t>(composition.frame_rate.numerator) : 60U;

        registry.composition_id_map[composition.id] = static_cast<std::uint32_t>(compiled.compositions.size());

        const std::string comp_path = "scene.compositions[" + composition.id + "]";
        
        // --- Component Expansion (Backgrounds/Presets) ---
        std::vector<LayerSpec> all_layers;
        
        for (const auto& inst : composition.component_instances) {
            // Search in local scene components first, then fallback to global library
            auto comp_it = std::find_if(composition.components.begin(), composition.components.end(),
                [&](const ComponentSpec& c) { return c.id == inst.component_id; });
            
            const ComponentSpec* component = nullptr;
            if (comp_it != composition.components.end()) {
                component = &(*comp_it);
            } else {
                component = ComponentLibrary::global().find_component(inst.component_id);
            }
            
            if (component) {
                for (const auto& layer_spec : component->layers) {
                    // Serialize to JSON for string-based parameter substitution
                    nlohmann::json layer_json = serialize_layer(layer_spec);
                    std::string layer_str = layer_json.dump();
                    
                    for (const auto& [name, value_json_str] : inst.param_values) {
                        std::string placeholder = "{{" + name + "}}";
                        std::string value = value_json_str;
                        if (value.size() >= 2 && value.front() == '"' && value.back() == '"') {
                            value = value.substr(1, value.size() - 2);
                        }
                        
                        size_t pos = 0;
                        while ((pos = layer_str.find(placeholder, pos)) != std::string::npos) {
                            layer_str.replace(pos, placeholder.length(), value);
                            pos += value.length();
                        }
                    }
                    
                    try {
                        nlohmann::json substituted_json = nlohmann::json::parse(layer_str);
                        LayerSpec expanded_layer;
                        parse_layer(substituted_json, expanded_layer, comp_path + ".components[" + inst.component_id + "]", diagnostics);
                        expanded_layer.id = inst.instance_id + "_" + expanded_layer.id;
                        all_layers.push_back(std::move(expanded_layer));
                    } catch (const std::exception& e) {
                        diagnostics.add_error("COMPILER_E005", "Failed to parse expanded component layer: " + std::string(e.what()), comp_path);
                    }
                }
            } else {
                diagnostics.add_warning("COMPILER_W006", "Component '" + inst.component_id + "' not found in scene or global library.", comp_path);
            }
        }

        // --- Add Explicit Scene Layers (on top of components) ---
        all_layers.insert(all_layers.end(), composition.layers.begin(), composition.layers.end());

        compiled_composition.layers.reserve(all_layers.size());
        for (std::size_t layer_idx = 0; layer_idx < all_layers.size(); ++layer_idx) {
            const auto& layer = all_layers[layer_idx];
            const std::string layer_path = comp_path + ".layers[" + std::to_string(layer_idx) + "]";

            if (layer.id.empty()) {
                diagnostics.add_error("COMPILER_E001",
                    "Layer at index " + std::to_string(layer_idx) + " has no id — skipping.",
                    layer_path);
                continue;
            }

            CompiledLayer compiled_layer = LayerCompiler::compile_layer(registry, layer, layer_path, scene, compiled, diagnostics);
            registry.layer_id_map[layer.id] = static_cast<std::uint32_t>(compiled_composition.layers.size());
            compiled_composition.layers.push_back(std::move(compiled_layer));
        }
        
        compiled_composition.camera_cuts = composition.camera_cuts;
        compiled_composition.audio_tracks = composition.audio_tracks;

        compiled.compositions.push_back(std::move(compiled_composition));
    }
}

} // namespace tachyon::detail
