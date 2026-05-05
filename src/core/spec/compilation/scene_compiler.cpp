#include "tachyon/core/spec/compilation/scene_compiler.h"
#include "tachyon/core/spec/compilation/scene_hash_builder.h"
#include "tachyon/core/spec/compilation/property_track_compiler.h"
#include "tachyon/importer/scene_importer.h"
#include "tachyon/core/string_utils.h"

#include <algorithm>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <type_traits>
#include <unordered_map>

namespace tachyon {
namespace {

} // namespace

SceneCompiler::SceneCompiler(SceneCompilerOptions options)
    : m_options(std::move(options)) {}

ResolutionResult<CompiledScene> SceneCompiler::compile(const SceneSpec& scene) const {
    ResolutionResult<CompiledScene> result;
    CompiledScene compiled;
    CompilationRegistry registry;

    compiled.header.version = 1;
    compiled.header.flags = static_cast<std::uint16_t>(m_options.determinism.fp_mode == DeterminismContract::FloatingPointMode::Strict ? 1U : 0U);
    compiled.header.layout_version = m_options.determinism.layout_version;
    compiled.header.compiler_version = m_options.determinism.compiler_version;
    compiled.header.layout_checksum = CompiledScene::calculate_layout_checksum();
    compiled.determinism = m_options.determinism;
    compiled.project_id = scene.project.id;
    compiled.project_name = scene.project.name;
    compiled.scene_hash = hash_scene_spec(scene, m_options.determinism);

    // 1. Build Compositions and Layers
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

        compiled_composition.layers.reserve(composition.layers.size());
        for (const auto& layer : composition.layers) {
            CompiledLayer compiled_layer;
            compiled_layer.node = registry.create_node(CompiledNodeType::Layer);
            compiled.graph.add_node(compiled_layer.node.node_id);
            
            // Resolve Type using robust LayerType enum
            auto type_map = [](LayerType k) -> std::uint32_t {
                switch (k) {
                    case LayerType::Solid:      return 1;
                    case LayerType::Shape:      return 2;
                    case LayerType::Image:      return 3;
                    case LayerType::Text:       return 4;
                    case LayerType::Precomp:    return 5;
                    case LayerType::Procedural: return 6;
                    case LayerType::Video:      return 3; // Video maps to Image for now
                    default: return 0;
                }
            };
            compiled_layer.type_id = type_map(layer.kind);
            
            compiled_layer.width = static_cast<std::uint32_t>(layer.width);
            compiled_layer.height = static_cast<std::uint32_t>(layer.height);
            compiled_layer.text_content = layer.text_content;
            compiled_layer.font_id = layer.font_id;
            compiled_layer.font_size = static_cast<float>(layer.font_size.value.has_value() ? *layer.font_size.value : 48.0);
            compiled_layer.text_alignment = layer.alignment == "center" ? 1 : (layer.alignment == "right" ? 2 : 0);
            compiled_layer.fill_color = layer.fill_color.value.has_value() ? *layer.fill_color.value : ColorSpec{255, 255, 255, 255};
            compiled_layer.stroke_color = layer.stroke_color.value.has_value() ? *layer.stroke_color.value : ColorSpec{255, 255, 255, 255};
            compiled_layer.stroke_width = layer.stroke_width_property.value.has_value() ? static_cast<float>(*layer.stroke_width_property.value) : static_cast<float>(layer.stroke_width);
            
            compiled_layer.shape_path = layer.shape_path;
            compiled_layer.effects = layer.effects;
            compiled_layer.animated_effects = layer.animated_effects;
            compiled_layer.procedural = layer.procedural;
            
            compiled_layer.mask_feather = static_cast<float>(layer.mask_feather.value.has_value() ? *layer.mask_feather.value : 0.0);
            compiled_layer.subtitle_path = layer.subtitle_path;
            compiled_layer.subtitle_outline_color = layer.subtitle_outline_color;
            compiled_layer.subtitle_outline_width = static_cast<float>(layer.subtitle_outline_width);
            compiled_layer.word_timestamp_path = layer.word_timestamp_path;
            
            if (layer.line_cap == "round") compiled_layer.line_cap = renderer2d::LineCap::Round;
            else if (layer.line_cap == "square") compiled_layer.line_cap = renderer2d::LineCap::Square;
            else compiled_layer.line_cap = renderer2d::LineCap::Butt;

            if (layer.line_join == "round") compiled_layer.line_join = renderer2d::LineJoin::Round;
            else if (layer.line_join == "bevel") compiled_layer.line_join = renderer2d::LineJoin::Bevel;
            else compiled_layer.line_join = renderer2d::LineJoin::Miter;

            compiled_layer.miter_limit = static_cast<float>(layer.miter_limit);
            
            compiled_layer.in_time = layer.in_point;
            compiled_layer.out_time = layer.out_point;
            compiled_layer.start_time = layer.start_time;
            compiled_layer.transition_in = layer.transition_in;
            compiled_layer.transition_out = layer.transition_out;
            
            // Build flags bitmask
            compiled_layer.flags = 0;
            if (layer.enabled) compiled_layer.flags |= 0x01;
            if (layer.visible) compiled_layer.flags |= 0x02;
            if (layer.is_3d) compiled_layer.flags |= 0x04;
            if (layer.is_adjustment_layer) compiled_layer.flags |= 0x08;
            if (layer.motion_blur) compiled_layer.flags |= 0x10;

            compiled_layer.matte_type = layer.track_matte_type;

            registry.layer_id_map[layer.id] = static_cast<std::uint32_t>(compiled_composition.layers.size());

            // Resolve Property Indices
            const auto add_track = [&](const std::string& suffix, const auto& spec, double fallback) {
                compiled_layer.property_indices.push_back(static_cast<std::uint32_t>(compiled.property_tracks.size()));
                auto track = compile_property_track(registry, suffix, layer.id, spec, fallback);
                compiled.graph.add_node(track.node.node_id);
                
                compiled.graph.add_edge(track.node.node_id, compiled_layer.node.node_id, true);
                compiled.property_tracks.push_back(std::move(track));
            };

            add_track(".opacity", layer.opacity_property, layer.opacity);
            add_track(".position_x", layer.transform.position_property, layer.transform.position_x.value_or(0.0));
            add_track(".position_y", layer.transform.position_property, layer.transform.position_y.value_or(0.0));
            add_track(".scale_x", layer.transform.scale_property, layer.transform.scale_x.value_or(1.0));
            add_track(".scale_y", layer.transform.scale_property, layer.transform.scale_y.value_or(1.0));
            add_track(".rotation", layer.transform.rotation_property, layer.transform.rotation.value_or(0.0));
            add_track(".mask_feather", layer.mask_feather, 0.0);

            // 3D Transforms
            add_track(".position_z", layer.transform3d.position_property, 0.0);
            add_track(".rotation_x", layer.transform3d.rotation_property, 0.0);
            add_track(".rotation_y", layer.transform3d.rotation_property, 0.0);
            add_track(".rotation_z", layer.transform3d.rotation_property, 0.0);
            add_track(".scale_z", layer.transform3d.scale_property, 1.0);
            add_track(".anchor_x", layer.transform3d.anchor_point_property, static_cast<double>(layer.width) * 0.5);
            add_track(".anchor_y", layer.transform3d.anchor_point_property, static_cast<double>(layer.height) * 0.5);
            add_track(".anchor_z", layer.transform3d.anchor_point_property, 0.0);

            // Material properties
            add_track(".metallic", layer.metallic, 0.0);
            add_track(".roughness", layer.roughness, 0.5);
            add_track(".ior", layer.ior, 1.45);
            add_track(".transmission", layer.transmission, 0.0);
            add_track(".emission_strength", layer.emission_strength, 0.0);
            
            compiled_layer.emission_color = layer.emission_color.value.has_value() ? *layer.emission_color.value : ColorSpec{0, 0, 0, 255};

            // Populate Unified Fields
            compiled_layer.track_bindings = layer.track_bindings;
            compiled_layer.time_remap = layer.time_remap;
            compiled_layer.frame_blend = layer.frame_blend;

            compiled_composition.layers.push_back(std::move(compiled_layer));
        }
        
        compiled_composition.camera_cuts = composition.camera_cuts;
        compiled_composition.audio_tracks = composition.audio_tracks;

        compiled.compositions.push_back(std::move(compiled_composition));
    }


    // 2. Resolve Multi-Node Dependencies (Parenting, Track Mattes, Precomps)
    for (std::uint32_t c_idx = 0; c_idx < compiled.compositions.size(); ++c_idx) {
        auto& comp = compiled.compositions[c_idx];
        const auto& comp_spec = scene.compositions[c_idx];
        
        for (std::uint32_t l_idx = 0; l_idx < comp.layers.size(); ++l_idx) {
            auto& layer = comp.layers[l_idx];
            const auto& layer_spec = comp_spec.layers[l_idx];

            if (layer_spec.parent.has_value()) {
                auto it = registry.layer_id_map.find(*layer_spec.parent);
                if (it != registry.layer_id_map.end()) {
                    layer.parent_index = it->second;
                    compiled.graph.add_edge(comp.layers[it->second].node.node_id, layer.node.node_id, true);
                }
            }

            if (layer_spec.track_matte_layer_id.has_value()) {
                auto it = registry.layer_id_map.find(*layer_spec.track_matte_layer_id);
                if (it != registry.layer_id_map.end()) {
                    layer.matte_layer_index = it->second;
                    compiled.graph.add_edge(comp.layers[it->second].node.node_id, layer.node.node_id, true);
                }
            }

            if (layer_spec.precomp_id.has_value()) {
                auto it = registry.composition_id_map.find(*layer_spec.precomp_id);
                if (it != registry.composition_id_map.end()) {
                    layer.precomp_index = it->second;
                    compiled.graph.add_edge(compiled.compositions[it->second].node.node_id, layer.node.node_id, true);
                }
            }
        }
    }

    // 3. Compile Graph and Assign Topological Indices
    compiled.graph.compile();
    const auto& topo = compiled.graph.topo_order();
    std::unordered_map<std::uint32_t, std::uint32_t> topo_map;
    for (std::uint32_t i = 0; i < topo.size(); ++i) {
        topo_map[topo[i]] = i;
    }

    // Update nodes with their topo_index
    for (auto& comp : compiled.compositions) {
        comp.node.topo_index = topo_map[comp.node.node_id];
        for (auto& layer : comp.layers) {
            layer.node.topo_index = topo_map[layer.node.node_id];
        }
    }
    for (auto& track : compiled.property_tracks) {
        track.node.topo_index = topo_map[track.node.node_id];
    }

    compiled.assets = scene.assets;
    compiled.link_dependency_nodes();
    result.value = std::move(compiled);
    return result;
}


bool SceneCompiler::update_compiled_scene(CompiledScene& existing, const SceneSpec& new_spec) const {
    // If the structural hash (topology) hasn't changed, we can do a fast property-only update.
    std::uint64_t new_hash = hash_scene_spec(new_spec, existing.determinism);
    if (new_hash == existing.scene_hash) {
        return true; 
    }

    auto result = compile(new_spec);
    if (result.ok()) {
        existing = std::move(*result.value);
        return true;
    }

    return false;
}
    
ResolutionResult<SceneSpec> SceneCompiler::import_external_scene(const std::filesystem::path& path) {
    ResolutionResult<SceneSpec> result;
    std::string ext = path.extension().string();
    ascii_lower_inplace(ext);

    std::unique_ptr<importer::SceneImporter> imp;
    if (ext == ".usd" || ext == ".usda" || ext == ".usdc") {
        imp = importer::create_usd_importer();
    } else if (ext == ".abc") {
        imp = importer::create_alembic_importer();
    }

    if (!imp) {
        result.diagnostics.add_error("IMPORT_ERR", "No importer available for format: " + ext);
        return result;
    }

    if (!imp->load(path.string())) {
        result.diagnostics.add_error("IMPORT_ERR", "Failed to load scene: " + imp->last_error());
        return result;
    }

    result.value = imp->build_scene_spec();
    return result;
}

} // namespace tachyon
