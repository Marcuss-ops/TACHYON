#include "tachyon/core/spec/compilation/scene_compiler.h"
#include "tachyon/runtime/cache/cache_key_builder.h"
#include "tachyon/importer/scene_importer.h"
#include "tachyon/core/expressions/expression_engine.h"

#include <algorithm>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <type_traits>
#include <unordered_map>
#include <nlohmann/json.hpp>

namespace tachyon {
namespace {

void add_string(CacheKeyBuilder& builder, const std::string& value) {
    builder.add_string(std::string_view{value});
}

template <typename T>
void add_json(CacheKeyBuilder& builder, const T& value) {
    builder.add_string(nlohmann::json(value).dump());
}

std::uint64_t hash_scene_spec(const SceneSpec& scene, const DeterminismContract& contract) {
    CacheKeyBuilder builder;
    builder.add_string(scene.version);
    builder.add_string(scene.spec_version);
    add_string(builder, scene.project.id);
    add_string(builder, scene.project.name);
    add_string(builder, scene.project.authoring_tool);
    if (scene.project.root_seed.has_value()) {
        builder.add_bool(true);
        builder.add_u64(static_cast<std::uint64_t>(*scene.project.root_seed));
    } else {
        builder.add_bool(false);
    }

    builder.add_u64(contract.fingerprint());
    builder.add_u64(static_cast<std::uint64_t>(scene.assets.size()));
    builder.add_u64(static_cast<std::uint64_t>(scene.compositions.size()));

    for (const auto& composition : scene.compositions) {
        add_string(builder, composition.id);
        add_string(builder, composition.name);
        builder.add_u64(static_cast<std::uint64_t>(composition.width));
        builder.add_u64(static_cast<std::uint64_t>(composition.height));
        builder.add_u64(static_cast<std::uint64_t>(composition.duration * 1000.0));
        builder.add_u64(static_cast<std::uint64_t>(composition.frame_rate.numerator));
        builder.add_u64(static_cast<std::uint64_t>(composition.frame_rate.denominator));
        builder.add_bool(composition.background.has_value());
        if (composition.background.has_value()) {
            add_string(builder, *composition.background);
        }

        builder.add_u64(static_cast<std::uint64_t>(composition.camera_cuts.size()));
        for (const auto& cut : composition.camera_cuts) {
            add_string(builder, cut.camera_id);
            builder.add_u64(static_cast<std::uint64_t>(cut.start_seconds * 1000.0));
            builder.add_u64(static_cast<std::uint64_t>(cut.end_seconds * 1000.0));
        }

        builder.add_u64(static_cast<std::uint64_t>(composition.audio_tracks.size()));
        for (const auto& track : composition.audio_tracks) {
            add_string(builder, track.id);
            add_string(builder, track.source_path);
            builder.add_u64(static_cast<std::uint64_t>(track.volume * 1000.0f));
            builder.add_u64(static_cast<std::uint64_t>(track.pan * 1000.0f));
            builder.add_u64(static_cast<std::uint64_t>(track.start_offset_seconds * 1000.0));
            
            builder.add_u64(static_cast<std::uint64_t>(track.volume_keyframes.size()));
            for (const auto& kf : track.volume_keyframes) {
                builder.add_u64(static_cast<std::uint64_t>(kf.time * 1000.0));
                builder.add_u64(static_cast<std::uint64_t>(kf.value * 1000.0f));
            }
            
            builder.add_u64(static_cast<std::uint64_t>(track.pan_keyframes.size()));
            for (const auto& kf : track.pan_keyframes) {
                builder.add_u64(static_cast<std::uint64_t>(kf.time * 1000.0));
                builder.add_u64(static_cast<std::uint64_t>(kf.value * 1000.0f));
            }

            builder.add_u64(static_cast<std::uint64_t>(track.effects.size()));
            for (const auto& effect : track.effects) {
                add_string(builder, effect.type);
                if (effect.gain_db) builder.add_u64(static_cast<std::uint64_t>(*effect.gain_db * 1000.0f));
                if (effect.cutoff_freq_hz) builder.add_u64(static_cast<std::uint64_t>(*effect.cutoff_freq_hz * 1000.0f));
            }
        }

        builder.add_u64(static_cast<std::uint64_t>(composition.layers.size()));
        for (const auto& layer : composition.layers) {
            add_string(builder, layer.id);
            add_string(builder, layer.name);
            add_string(builder, layer.type);
            add_string(builder, layer.blend_mode);
            builder.add_bool(layer.enabled);
            builder.add_bool(layer.visible);
            builder.add_bool(layer.is_3d);
            builder.add_bool(layer.is_adjustment_layer);
            builder.add_u64(static_cast<std::uint64_t>(layer.start_time * 1000.0));
            builder.add_u64(static_cast<std::uint64_t>(layer.in_point * 1000.0));
            builder.add_u64(static_cast<std::uint64_t>(layer.out_point * 1000.0));
            builder.add_u64(static_cast<std::uint64_t>(layer.opacity * 1000000.0));
            builder.add_u64(static_cast<std::uint64_t>(layer.extrusion_depth * 1000.0));
            builder.add_u64(static_cast<std::uint64_t>(layer.bevel_size * 1000.0));
            builder.add_u64(static_cast<std::uint64_t>(layer.hole_bevel_ratio * 1000.0));
            
            builder.add_bool(layer.transform.position_x.has_value());
            if (layer.transform.position_x) builder.add_u64(static_cast<std::uint64_t>(*layer.transform.position_x * 1000.0));
            builder.add_bool(layer.transform.position_y.has_value());
            if (layer.transform.position_y) builder.add_u64(static_cast<std::uint64_t>(*layer.transform.position_y * 1000.0));
            builder.add_bool(layer.transform.scale_x.has_value());
            if (layer.transform.scale_x) builder.add_u64(static_cast<std::uint64_t>(*layer.transform.scale_x * 1000.0));
            builder.add_bool(layer.transform.scale_y.has_value());
            if (layer.transform.scale_y) builder.add_u64(static_cast<std::uint64_t>(*layer.transform.scale_y * 1000.0));
            builder.add_bool(layer.transform.rotation.has_value());
            if (layer.transform.rotation) builder.add_u64(static_cast<std::uint64_t>(*layer.transform.rotation * 1000.0));
            builder.add_bool(layer.mask_feather.value.has_value());
            if (layer.mask_feather.value) builder.add_u64(static_cast<std::uint64_t>(*layer.mask_feather.value * 1000.0));
            builder.add_bool(layer.mask_feather.expression.has_value());
            if (layer.mask_feather.expression) add_string(builder, *layer.mask_feather.expression);

            builder.add_bool(layer.opacity_property.expression.has_value());
            if (layer.opacity_property.expression) add_string(builder, *layer.opacity_property.expression);
            builder.add_bool(layer.transform.position_property.expression.has_value());
            if (layer.transform.position_property.expression) add_string(builder, *layer.transform.position_property.expression);
            builder.add_bool(layer.transform.scale_property.expression.has_value());
            if (layer.transform.scale_property.expression) add_string(builder, *layer.transform.scale_property.expression);
            builder.add_bool(layer.transform.rotation_property.expression.has_value());
            if (layer.transform.rotation_property.expression) add_string(builder, *layer.transform.rotation_property.expression);

            builder.add_bool(layer.parent.has_value());
            if (layer.parent.has_value()) {
                add_string(builder, *layer.parent);
            }

            builder.add_u64(static_cast<std::uint64_t>(layer.text_animators.size()));
            for (const auto& animator : layer.text_animators) {
                add_json(builder, animator);
            }
            builder.add_u64(static_cast<std::uint64_t>(layer.text_highlights.size()));
            for (const auto& highlight : layer.text_highlights) {
                add_json(builder, highlight);
            }

            // Unified Temporal & Tracking Hashing
            builder.add_u64(static_cast<std::uint64_t>(layer.track_bindings.size()));
            for (const auto& binding : layer.track_bindings) {
                add_string(builder, binding.property_path);
                add_string(builder, binding.source_id);
                add_string(builder, binding.source_track_name);
                builder.add_u64(static_cast<std::uint64_t>(binding.influence * 1000.0f));
                builder.add_bool(binding.enabled);
            }
            builder.add_bool(layer.time_remap.enabled);
            builder.add_u64(static_cast<std::uint64_t>(layer.time_remap.mode));
            builder.add_u64(static_cast<std::uint64_t>(layer.time_remap.keyframes.size()));
            for (const auto& kf : layer.time_remap.keyframes) {
                builder.add_u64(static_cast<std::uint64_t>(kf.first * 1000.0f));
                builder.add_u64(static_cast<std::uint64_t>(kf.second * 1000.0f));
            }
            builder.add_u64(static_cast<std::uint64_t>(layer.frame_blend));
        }
    }

    return builder.finish();
}

/**
 * @brief Context used during compilation to track node IDs and dependencies.
 */
struct CompilationRegistry {
    std::uint32_t next_id{1};
    std::unordered_map<std::string, std::uint32_t> layer_id_map;
    std::unordered_map<std::string, std::uint32_t> composition_id_map;
    
    CompiledNode create_node(CompiledNodeType type) {
        CompiledNode node;
        node.node_id = next_id++;
        node.type = type;
        return node;
    }
};

template<typename T>
CompiledPropertyTrack compile_property_track(
    CompilationRegistry& registry,
    const std::string& id_suffix, 
    const std::string& layer_id, 
    const T& property_spec, 
    double fallback_value,
    std::vector<expressions::Bytecode>& expressions) {
    
    CompiledPropertyTrack track;
    track.node = registry.create_node(CompiledNodeType::Property);
    track.property_id = layer_id + id_suffix;

    if constexpr (std::is_same_v<T, AnimatedVector2Spec>) {
        if (property_spec.value.has_value()) {
            if (id_suffix.find("_x") != std::string::npos) track.constant_value = property_spec.value->x;
            else if (id_suffix.find("_y") != std::string::npos) track.constant_value = property_spec.value->y;
            else track.constant_value = fallback_value;
        } else {
            track.constant_value = fallback_value;
        }
    } else {
        track.constant_value = property_spec.value.has_value() ? static_cast<double>(*property_spec.value) : fallback_value;
    }

    if (!property_spec.keyframes.empty()) {
        track.kind = CompiledPropertyTrack::Kind::Keyframed;
        track.keyframes.reserve(property_spec.keyframes.size());
        for (std::size_t i = 0; i < property_spec.keyframes.size(); ++i) {
            const auto& keyframe = property_spec.keyframes[i];
            double val = 0.0;
            if constexpr (std::is_same_v<T, AnimatedVector2Spec>) {
                if (id_suffix.find("_x") != std::string::npos) val = keyframe.value.x;
                else if (id_suffix.find("_y") != std::string::npos) val = keyframe.value.y;
            } else {
                val = static_cast<double>(keyframe.value);
            }

            CompiledKeyframe ck;
            ck.time = keyframe.time;
            ck.value = val;
            ck.easing = static_cast<std::uint32_t>(keyframe.easing);
            ck.cx1 = keyframe.bezier.cx1;
            ck.cy1 = keyframe.bezier.cy1;
            ck.cx2 = keyframe.bezier.cx2;
            ck.cy2 = keyframe.bezier.cy2;

            // If it's a custom easing and we have a next keyframe, we can compute the AE-style Bezier
            if (keyframe.easing == animation::EasingPreset::Custom && i + 1 < property_spec.keyframes.size()) {
                const auto& next_kf = property_spec.keyframes[i + 1];
                double next_val = 0.0;
                if constexpr (std::is_same_v<T, AnimatedVector2Spec>) {
                    if (id_suffix.find("_x") != std::string::npos) next_val = next_kf.value.x;
                    else if (id_suffix.find("_y") != std::string::npos) next_val = next_kf.value.y;
                } else {
                    next_val = static_cast<double>(next_kf.value);
                }

                double duration = next_kf.time - keyframe.time;
                double delta = next_val - val;
                
                // AE handles are often specified via speed/influence. 
                // We use from_ae to convert them if they seem to be set (influence != 0).
                if (duration > 1e-6 && (keyframe.influence_out > 0.0 || next_kf.influence_in > 0.0)) {
                    auto ae_bezier = animation::CubicBezierEasing::from_ae(
                        keyframe.speed_out, keyframe.influence_out,
                        next_kf.speed_in, next_kf.influence_in,
                        duration, delta
                    );
                    ck.cx1 = ae_bezier.cx1;
                    ck.cy1 = ae_bezier.cy1;
                    ck.cx2 = ae_bezier.cx2;
                    ck.cy2 = ae_bezier.cy2;
                }
            }

            track.keyframes.push_back(ck);
        }
    } else {
        track.kind = CompiledPropertyTrack::Kind::Constant;
    }

    if (property_spec.expression.has_value() && !property_spec.expression->empty()) {
        track.kind = CompiledPropertyTrack::Kind::Expression;
        auto compile_result = expressions::CoreExpressionEvaluator::compile(*property_spec.expression);
        if (compile_result.success) {
            track.expression_index = static_cast<std::uint32_t>(expressions.size());
            expressions.push_back(std::move(compile_result.bytecode));
        } else {
            // Fallback to constant value if compilation fails
            track.kind = CompiledPropertyTrack::Kind::Constant;
        }
    }

    return track;
}

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
            
            // Resolve Type
            auto type_map = [](const std::string& t) -> std::uint32_t {
                if (t == "solid") return 1;
                if (t == "shape") return 2;
                if (t == "image") return 3;
                if (t == "text") return 4;
                if (t == "precomp") return 5;
                return 0;
            };
            compiled_layer.type_id = type_map(layer.type);
            
            compiled_layer.width = static_cast<std::uint32_t>(layer.width);
            compiled_layer.height = static_cast<std::uint32_t>(layer.height);
            compiled_layer.text_content = layer.text_content;
            compiled_layer.font_id = layer.font_id;
            compiled_layer.font_size = static_cast<float>(layer.font_size.value.has_value() ? *layer.font_size.value : 48.0);
            compiled_layer.text_alignment = layer.alignment == "center" ? 1 : (layer.alignment == "right" ? 2 : 0);
            compiled_layer.fill_color = layer.fill_color.value.has_value() ? *layer.fill_color.value : ColorSpec{255, 255, 255, 255};
            compiled_layer.stroke_color = layer.stroke_color.value.has_value() ? *layer.stroke_color.value : ColorSpec{255, 255, 255, 255};
            compiled_layer.stroke_width = layer.stroke_width_property.value.has_value() ? static_cast<float>(*layer.stroke_width_property.value) : static_cast<float>(layer.stroke_width);
            compiled_layer.extrusion_depth = static_cast<float>(layer.extrusion_depth);
            compiled_layer.bevel_size = static_cast<float>(layer.bevel_size);
            compiled_layer.hole_bevel_ratio = static_cast<float>(layer.hole_bevel_ratio);
            
            compiled_layer.shape_path = layer.shape_path;
            compiled_layer.effects = layer.effects;
            compiled_layer.animated_effects = layer.animated_effects;
            compiled_layer.text_animators = layer.text_animators;
            compiled_layer.text_highlights = layer.text_highlights;
            
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
                auto track = compile_property_track(registry, suffix, layer.id, spec, fallback, compiled.expressions);
                compiled.graph.add_node(track.node.node_id);
                
                // DATA BINDING INTEGRATION:
                // If the track has a binding in the spec (not actually in SceneSpec yet, added for future-proofing Step 4)
                // track.binding = resolve_binding(layer, suffix); 
                
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
            add_track(".anchor_point_x", layer.transform.anchor_point, layer.transform.anchor_point.value.has_value() ? layer.transform.anchor_point.value->x : 0.0);
            add_track(".anchor_point_y", layer.transform.anchor_point, layer.transform.anchor_point.value.has_value() ? layer.transform.anchor_point.value->y : 0.0);

            // Populate Unified Fields
            compiled_layer.track_bindings = layer.track_bindings;
            compiled_layer.time_remap = layer.time_remap;
            compiled_layer.frame_blend = layer.frame_blend;

            // Populate temporal bounds and blend mode from SceneSpec
            compiled_layer.in_time = layer.in_point;
            compiled_layer.out_time = layer.out_point;
            compiled_layer.start_time = layer.start_time;
            compiled_layer.blend_mode = layer.blend_mode;

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

    // For a real industrial version, we would compare layer counts, IDs, etc.
    // For Milestone 1, we replace the entire content but allow the CLI to keep the 
    // CompiledScene object reference alive.
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
    std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return std::tolower(c); });

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
