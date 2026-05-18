#include "tachyon/runtime/compiler/scene_compiler.h"
#include "tachyon/runtime/compiler/scene_hash_builder.h"
#include "tachyon/runtime/compiler/property_track_compiler.h"
#include "tachyon/core/spec/validation/scene_normalizer.h"
#include "tachyon/core/string_utils.h"

#include <algorithm>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <type_traits>
#include <unordered_map>
#include <iostream>

namespace tachyon {
namespace {

static TemporalStability analyze_layer_stability(
    std::uint32_t c_idx,
    std::uint32_t l_idx,
    CompiledScene& compiled,
    std::vector<std::vector<TemporalStability>>& memo,
    std::vector<std::vector<bool>>& visiting) {
    
    if (visiting[c_idx][l_idx]) {
        return TemporalStability::Static; // Break dependency cycles gracefully
    }
    
    if (memo[c_idx][l_idx] != TemporalStability::Unknown) {
        return memo[c_idx][l_idx];
    }

    auto& comp = compiled.compositions[c_idx];
    auto& layer = comp.layers[l_idx];

    // Video layers are dynamic because they depend on media time
    if (layer.type_id == 8) { // 8 is Video
        memo[c_idx][l_idx] = TemporalStability::DependsOnMediaTime;
        return TemporalStability::DependsOnMediaTime;
    }

    // Check if any of the properties (tracks) of the layer is animated
    for (auto track_idx : layer.property_indices) {
        if (track_idx < compiled.property_tracks.size()) {
            const auto& track = compiled.property_tracks[track_idx];
            if (track.kind != CompiledPropertyTrack::Kind::Constant) {
                memo[c_idx][l_idx] = TemporalStability::Animated;
                return TemporalStability::Animated;
            }
        }
    }

    // Check if layer has active effects or text animators
    if (!layer.effects.empty() || !layer.text_animators.empty()) {
        memo[c_idx][l_idx] = TemporalStability::Animated;
        return TemporalStability::Animated;
    }

    // Check if layer has track bindings
    if (!layer.track_bindings.empty()) {
        memo[c_idx][l_idx] = TemporalStability::Animated;
        return TemporalStability::Animated;
    }

    visiting[c_idx][l_idx] = true;

    // Check parent layer stability
    if (layer.parent_index.has_value()) {
        auto parent_stability = analyze_layer_stability(c_idx, *layer.parent_index, compiled, memo, visiting);
        if (parent_stability != TemporalStability::Static) {
            visiting[c_idx][l_idx] = false;
            memo[c_idx][l_idx] = parent_stability;
            return parent_stability;
        }
    }

    // Check track matte layer stability
    if (layer.matte_layer_index.has_value()) {
        auto matte_stability = analyze_layer_stability(c_idx, *layer.matte_layer_index, compiled, memo, visiting);
        if (matte_stability != TemporalStability::Static) {
            visiting[c_idx][l_idx] = false;
            memo[c_idx][l_idx] = matte_stability;
            return matte_stability;
        }
    }

    // Check precomp composition stability (if it's a nested composition)
    if (layer.precomp_index.has_value() && *layer.precomp_index < compiled.compositions.size()) {
        auto nested_comp_idx = *layer.precomp_index;
        const auto& nested_comp = compiled.compositions[nested_comp_idx];
        bool nested_static = true;
        TemporalStability worst_stability = TemporalStability::Static;
        for (std::uint32_t nl_idx = 0; nl_idx < nested_comp.layers.size(); ++nl_idx) {
            auto nested_stability = analyze_layer_stability(nested_comp_idx, nl_idx, compiled, memo, visiting);
            if (nested_stability != TemporalStability::Static) {
                nested_static = false;
                worst_stability = nested_stability;
                break;
            }
        }
        if (!nested_static) {
            visiting[c_idx][l_idx] = false;
            memo[c_idx][l_idx] = worst_stability;
            return worst_stability;
        }
    }

    visiting[c_idx][l_idx] = false;
    memo[c_idx][l_idx] = TemporalStability::Static;
    return TemporalStability::Static;
}

} // namespace


SceneCompiler::SceneCompiler(SceneCompilerOptions options)
    : m_options(std::move(options)) {}

ResolutionResult<CompiledScene> SceneCompiler::compile(const SceneSpec& scene_spec) const {
    // Take a copy to normalize
    SceneSpec scene = scene_spec;
    core::SceneNormalizer::normalize(scene);

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
        compiled_composition.composition_id = composition.id;
        compiled_composition.name = composition.name;
        compiled.graph.add_node(compiled_composition.node.node_id);
        compiled_composition.width = static_cast<std::uint32_t>(std::max<std::int64_t>(0, composition.width));
        compiled_composition.height = static_cast<std::uint32_t>(std::max<std::int64_t>(0, composition.height));
        compiled_composition.duration = composition.duration;
        compiled_composition.fps = composition.frame_rate.numerator > 0 ? static_cast<std::uint32_t>(composition.frame_rate.numerator) : 60U;

        registry.composition_id_map[composition.id] = static_cast<std::uint32_t>(compiled.compositions.size());

        compiled_composition.layers.reserve(composition.layers.size() + (composition.background.has_value() ? 1 : 0));
        
        // Resolve Background into a standard layer (converged domain)
        if (composition.background.has_value()) {
            CompiledLayer bg_layer;
            bg_layer.node = registry.create_node(CompiledNodeType::Layer);
            bg_layer.name = "Background";
            compiled.graph.add_node(bg_layer.node.node_id);
            
            if (composition.background->type == BackgroundType::Color) {
                bg_layer.type_id = compiled_type_id_from_layer_type(LayerType::Solid);
                bg_layer.text.fill_color = composition.background->get_color().value_or(ColorSpec{255,255,255,255});
            } else if (composition.background->type == BackgroundType::Asset) {
                bg_layer.type_id = compiled_type_id_from_layer_type(LayerType::Image);
                bg_layer.name = composition.background->value;
                bg_layer.asset_path = composition.background->value;
            } else if (composition.background->type == BackgroundType::Preset || composition.background->type == BackgroundType::Component) {
                bg_layer.type_id = compiled_type_id_from_layer_type(LayerType::Procedural);
                bg_layer.name = composition.background->value;
                bg_layer.procedural.emplace();
                bg_layer.procedural->kind = composition.background->value;
            } else {
                bg_layer.type_id = compiled_type_id_from_layer_type(LayerType::Solid);
                bg_layer.text.fill_color = ColorSpec{0,0,0,255};
            }
            
            bg_layer.width = compiled_composition.width;
            bg_layer.height = compiled_composition.height;
            bg_layer.in_time = 0.0;
            bg_layer.out_time = compiled_composition.duration;
            bg_layer.flags = 0x01 | 0x02; // enabled | visible
            
            compiled_composition.layers.push_back(std::move(bg_layer));
        }

        for (const auto& layer : composition.layers) {
            CompiledLayer compiled_layer;
            compiled_layer.node = registry.create_node(CompiledNodeType::Layer);
            compiled_layer.name = layer.identity.name;
            
            // Resolve Source via variant
            std::visit([&](auto&& source) {
                using T = std::decay_t<decltype(source)>;
                if constexpr (std::is_same_v<T, MediaSource>) {
                    compiled_layer.asset_path = source.asset.id;
                } else if constexpr (std::is_same_v<T, ProceduralSource>) {
                    compiled_layer.procedural = source.spec;
                }
            }, layer.source);
            compiled.graph.add_node(compiled_layer.node.node_id);
            
            // Resolve Type using robust LayerType enum (Canonical Source of Truth)
            compiled_layer.type_id = compiled_type_id_from_layer_type(layer.identity.type);
            
            compiled_layer.width = static_cast<std::uint32_t>(layer.transform.width);
            compiled_layer.height = static_cast<std::uint32_t>(layer.transform.height);
            compiled_layer.text.content = layer.text.content;
            compiled_layer.text.font_id = layer.text.font_id;
            compiled_layer.text.font_size = static_cast<float>(layer.text.font_size.value.has_value() ? *layer.text.font_size.value : 48.0);
            compiled_layer.text.box = layer.text.box;
            compiled_layer.text.fill_color = layer.text.fill_color.value.has_value() ? *layer.text.fill_color.value : ColorSpec{255, 255, 255, 255};
            compiled_layer.text.stroke_color = layer.text.stroke_color.value.has_value() ? *layer.text.stroke_color.value : ColorSpec{255, 255, 255, 255};
            compiled_layer.text.stroke_width = layer.text.stroke_width_property.value.has_value() ? static_cast<float>(*layer.text.stroke_width_property.value) : static_cast<float>(layer.text.stroke_width);
            compiled_layer.text_animators = layer.text_animators;
            compiled_layer.text_highlights = layer.text_highlights;
            
            compiled_layer.vector.shape_path = layer.vector.shape_path;
            compiled_layer.effects = layer.effects;
            
            compiled_layer.masks.feather = static_cast<float>(layer.masks.feather.value.has_value() ? *layer.masks.feather.value : 0.0);
            compiled_layer.subtitles.path = layer.subtitles.path;
            compiled_layer.subtitles.outline_color = layer.subtitles.outline_color;
            compiled_layer.subtitles.outline_width = static_cast<float>(layer.subtitles.outline_width);
            compiled_layer.subtitles.word_timestamp_path = layer.subtitles.word_timestamp_path;
            
            switch (layer.vector.line_cap) {
                case spec::LineCap::Round: compiled_layer.vector.line_cap = renderer2d::LineCap::Round; break;
                case spec::LineCap::Square: compiled_layer.vector.line_cap = renderer2d::LineCap::Square; break;
                default: compiled_layer.vector.line_cap = renderer2d::LineCap::Butt; break;
            }

            switch (layer.vector.line_join) {
                case spec::LineJoin::Round: compiled_layer.vector.line_join = renderer2d::LineJoin::Round; break;
                case spec::LineJoin::Bevel: compiled_layer.vector.line_join = renderer2d::LineJoin::Bevel; break;
                default: compiled_layer.vector.line_join = renderer2d::LineJoin::Miter; break;
            }

            compiled_layer.vector.miter_limit = static_cast<float>(layer.vector.miter_limit);
            
            // Build flags bitmask
            compiled_layer.flags = 0;
            if (layer.identity.enabled) compiled_layer.flags |= 0x01;
            if (layer.identity.visible) compiled_layer.flags |= 0x02;
            if (layer.identity.is_adjustment_layer) compiled_layer.flags |= 0x08;
            if (layer.identity.motion_blur) compiled_layer.flags |= 0x10;

            compiled_layer.matte_type = layer.track_matte_type;

            registry.layer_id_map[layer.identity.id] = static_cast<std::uint32_t>(compiled_composition.layers.size());

            // Resolve Property Indices
            const auto add_track = [&](const std::string& suffix, const auto& spec, double fallback) {
                compiled_layer.property_indices.push_back(static_cast<std::uint32_t>(compiled.property_tracks.size()));
                auto track = compile_property_track(registry, suffix, layer.identity.id, spec, fallback);
                compiled.graph.add_node(track.node.node_id);
                
                compiled.graph.add_edge(track.node.node_id, compiled_layer.node.node_id, true);
                compiled.property_tracks.push_back(std::move(track));
            };

            // Properties (must match CompiledLayer::PropertyIndex order exactly)
            // 0: Opacity
            add_track(".opacity", layer.transform.opacity_property, layer.transform.opacity);
            // 1-2: Position 2D
            add_track(".position_x", layer.transform.transform.position_property, layer.transform.transform.position_x.value_or(0.0));
            add_track(".position_y", layer.transform.transform.position_property, layer.transform.transform.position_y.value_or(0.0));
            // 3-4: Scale 2D
            add_track(".scale_x", layer.transform.transform.scale_property, layer.transform.transform.scale_x.value_or(100.0));
            add_track(".scale_y", layer.transform.transform.scale_property, layer.transform.transform.scale_y.value_or(100.0));
            // 5: Rotation
            add_track(".rotation", layer.transform.transform.rotation_property, layer.transform.transform.rotation.value_or(0.0));
            // 6: Mask Feather
            add_track(".masks.feather", layer.masks.feather, 0.0);
            
            // Anchor points
            add_track(".anchor_point_x", layer.transform.transform.anchor_point, 0.0);
            add_track(".anchor_point_y", layer.transform.transform.anchor_point, 0.0);

            // Additional canonical tracks
            add_track(".playback.temporal.time_remap", layer.playback.temporal.time_remap_property, 0.0);
            add_track(".text.font_size", layer.text.font_size, 48.0);
            add_track(".text.stroke_width", layer.text.stroke_width_property, layer.text.stroke_width);
            add_track(".repeater.count", layer.repeater.count, 1.0);

            // Populate Unified Fields
            compiled_layer.track_bindings = layer.playback.temporal.track_bindings;
            compiled_layer.time_remap = layer.playback.temporal.time_remap;
            compiled_layer.frame_blend = layer.playback.temporal.frame_blend;

            compiled_layer.in_time = layer.playback.timing.source_in;
            compiled_layer.out_time = layer.playback.timing.source_out;
            compiled_layer.start_time = layer.playback.timing.start;
            compiled_layer.blend_mode = layer.blend_mode;
            compiled_layer.transition_in = layer.transition_in;
            compiled_layer.transition_out = layer.transition_out;

            compiled_composition.layers.push_back(std::move(compiled_layer));
        }
        
        compiled_composition.audio_tracks = composition.audio_tracks;

        // Compile Cameras
        compiled_composition.cameras.reserve(composition.cameras_2d.size());
        for (const auto& cam_spec : composition.cameras_2d) {
            CompiledCamera compiled_cam;
            compiled_cam.node = registry.create_node(CompiledNodeType::Track); // Using Track type for camera nodes for now
            compiled_cam.id = cam_spec.id;
            compiled_cam.name = cam_spec.name;
            compiled_cam.enabled = cam_spec.enabled;
            compiled.graph.add_node(compiled_cam.node.node_id);
            
            const auto add_cam_track = [&](const std::string& suffix, const auto& spec, double fallback) {
                compiled_cam.property_indices.push_back(static_cast<std::uint32_t>(compiled.property_tracks.size()));
                auto track = compile_property_track(registry, suffix, cam_spec.id, spec, fallback);
                compiled.graph.add_node(track.node.node_id);
                compiled.graph.add_edge(track.node.node_id, compiled_cam.node.node_id, true);
                compiled.property_tracks.push_back(std::move(track));
            };

            add_cam_track(".zoom", cam_spec.zoom, 1.0);
            add_cam_track(".position_x", cam_spec.position, static_cast<double>(composition.width) / 2.0);
            add_cam_track(".position_y", cam_spec.position, static_cast<double>(composition.height) / 2.0);
            add_cam_track(".rotation", cam_spec.rotation, 0.0);
            add_cam_track(".scale_x", cam_spec.scale, 100.0);
            add_cam_track(".scale_y", cam_spec.scale, 100.0);
            add_cam_track(".anchor_x", cam_spec.anchor_point, 0.0);
            add_cam_track(".anchor_y", cam_spec.anchor_point, 0.0);

            if (composition.active_camera2d_id.has_value() && *composition.active_camera2d_id == cam_spec.id) {
                compiled_composition.active_camera_index = static_cast<std::uint32_t>(compiled_composition.cameras.size());
            }

            compiled_composition.cameras.push_back(std::move(compiled_cam));
        }
        
        if (!compiled_composition.active_camera_index.has_value() && !compiled_composition.cameras.empty()) {
            compiled_composition.active_camera_index = 0;
        }

        compiled.compositions.push_back(std::move(compiled_composition));
    }


    // 2. Resolve Multi-Node Dependencies (Parenting, Track Mattes, Precomps)
    for (std::uint32_t c_idx = 0; c_idx < compiled.compositions.size(); ++c_idx) {
        auto& comp = compiled.compositions[c_idx];
        const auto& comp_spec = scene.compositions[c_idx];
        bool has_bg = comp_spec.background.has_value();
        
        for (std::uint32_t l_idx = 0; l_idx < comp_spec.layers.size(); ++l_idx) {
            std::uint32_t compiled_l_idx = l_idx + (has_bg ? 1 : 0);
            auto& layer = comp.layers[compiled_l_idx];
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

            if (const auto* precomp = std::get_if<PrecompSource>(&layer_spec.source)) {
                auto it = registry.composition_id_map.find(precomp->precomp_id);
                if (it != registry.composition_id_map.end()) {
                    layer.precomp_index = it->second;
                    compiled.graph.add_edge(compiled.compositions[it->second].node.node_id, layer.node.node_id, true);
                }
            }
        }
    }

    // 2b. Analyze Temporal Stability for all layers
    {
        std::vector<std::vector<TemporalStability>> memo(compiled.compositions.size());
        std::vector<std::vector<bool>> visiting(compiled.compositions.size());
        for (std::uint32_t c_idx = 0; c_idx < compiled.compositions.size(); ++c_idx) {
            memo[c_idx].resize(compiled.compositions[c_idx].layers.size(), TemporalStability::Unknown);
            visiting[c_idx].resize(compiled.compositions[c_idx].layers.size(), false);
        }
        for (std::uint32_t c_idx = 0; c_idx < compiled.compositions.size(); ++c_idx) {
            for (std::uint32_t l_idx = 0; l_idx < compiled.compositions[c_idx].layers.size(); ++l_idx) {
                compiled.compositions[c_idx].layers[l_idx].temporal_stability =
                    analyze_layer_stability(c_idx, l_idx, compiled, memo, visiting);
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
        for (auto& cam : comp.cameras) {
            cam.node.topo_index = topo_map[cam.node.node_id];
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
    result.diagnostics.add_error("IMPORT_ERR", "External scene import is currently disabled in this build.");
    return result;
}

} // namespace tachyon
