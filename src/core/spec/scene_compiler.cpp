#include "tachyon/core/spec/scene_compiler.h"
#include "tachyon/runtime/cache/cache_key_builder.h"

#include <algorithm>
#include <cstdint>
#include <string_view>
#include <utility>
#include <type_traits>
#include <unordered_map>

namespace tachyon {
namespace {

void add_string(CacheKeyBuilder& builder, const std::string& value) {
    builder.add_string(std::string_view{value});
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
    double fallback_value = 0.0) {
    
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
        for (const auto& keyframe : property_spec.keyframes) {
            double val = 0.0;
            if constexpr (std::is_same_v<T, AnimatedVector2Spec>) {
                if (id_suffix.find("_x") != std::string::npos) val = keyframe.value.x;
                else if (id_suffix.find("_y") != std::string::npos) val = keyframe.value.y;
            } else {
                val = static_cast<double>(keyframe.value);
            }
            track.keyframes.push_back(CompiledKeyframe{keyframe.time, val, static_cast<std::uint32_t>(keyframe.easing)});
        }
    } else {
        track.kind = CompiledPropertyTrack::Kind::Constant;
    }

    if (property_spec.expression.has_value() && !property_spec.expression->empty()) {
        track.kind = CompiledPropertyTrack::Kind::Expression;
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
        compiled_composition.id = composition.id;
        compiled_composition.name = composition.name;
        compiled_composition.width = static_cast<std::uint32_t>(std::max<std::int64_t>(0, composition.width));
        compiled_composition.height = static_cast<std::uint32_t>(std::max<std::int64_t>(0, composition.height));
        compiled_composition.duration = composition.duration;
        compiled_composition.fps = composition.frame_rate.numerator > 0 ? static_cast<std::uint32_t>(composition.frame_rate.numerator) : 60U;

        registry.composition_id_map[composition.id] = compiled_composition.node.node_id;

        compiled_composition.layers.reserve(composition.layers.size());
        for (const auto& layer : composition.layers) {
            CompiledLayer compiled_layer;
            compiled_layer.node = registry.create_node(CompiledNodeType::Layer);
            compiled_layer.id = layer.id;
            compiled_layer.name = layer.name;
            compiled_layer.type = layer.type;
            compiled_layer.blend_mode = layer.blend_mode;
            compiled_layer.enabled = layer.enabled;
            compiled_layer.visible = layer.visible;
            compiled_layer.is_3d = layer.is_3d;
            compiled_layer.is_adjustment_layer = layer.is_adjustment_layer;
            compiled_layer.start_time = layer.start_time;
            compiled_layer.in_point = layer.in_point;
            compiled_layer.out_point = layer.out_point;
            compiled_layer.opacity = layer.opacity_property.value.has_value() ? *layer.opacity_property.value : layer.opacity;

            registry.layer_id_map[layer.id] = compiled_layer.node.node_id;

            // Simple property tracks (Opacity, Position, Scale, Rotation)
            const auto add_track = [&](const std::string& suffix, const auto& spec, double fallback) {
                compiled_layer.property_indices.push_back(static_cast<std::uint32_t>(compiled.property_tracks.size()));
                auto track = compile_property_track(registry, suffix, layer.id, spec, fallback);
                
                // Add dependency: Layer depends on Property
                compiled.graph.add_edge(track.node.node_id, compiled_layer.node.node_id, true);
                compiled.property_tracks.push_back(std::move(track));
            };

            add_track(".opacity", layer.opacity_property, layer.opacity);
            add_track(".position_x", layer.transform.position_property, layer.transform.position_x.value_or(0.0));
            add_track(".position_y", layer.transform.position_property, layer.transform.position_y.value_or(0.0));
            add_track(".scale_x", layer.transform.scale_property, layer.transform.scale_x.value_or(1.0));
            add_track(".scale_y", layer.transform.scale_property, layer.transform.scale_y.value_or(1.0));
            add_track(".rotation", layer.transform.rotation_property, layer.transform.rotation.value_or(0.0));

            compiled_composition.layers.push_back(std::move(compiled_layer));
        }

        compiled.compositions.push_back(std::move(compiled_composition));
    }

    // 2. Resolve Multi-Node Dependencies (Parenting, Track Mattes, Precomps)
    for (auto& comp : compiled.compositions) {
        for (auto& layer : comp.layers) {
            // Find the original layer spec to get parenting info
            const auto& comp_spec = *std::find_if(scene.compositions.begin(), scene.compositions.end(), [&](const auto& c) { return c.id == comp.id; });
            const auto& layer_spec = *std::find_if(comp_spec.layers.begin(), comp_spec.layers.end(), [&](const auto& l) { return l.id == layer.id; });

            if (layer_spec.parent.has_value()) {
                auto it = registry.layer_id_map.find(*layer_spec.parent);
                if (it != registry.layer_id_map.end()) {
                    compiled.graph.add_edge(it->second, layer.node.node_id, true);
                    
                    // Track parent index for legacy reasons (if needed)
                    auto parent_it = std::find_if(comp_spec.layers.begin(), comp_spec.layers.end(), [&](const auto& l) { return l.id == *layer_spec.parent; });
                    if (parent_it != comp_spec.layers.end()) {
                         layer.parent_index = static_cast<std::uint32_t>(std::distance(comp_spec.layers.begin(), parent_it));
                    }
                }
            }

            if (layer_spec.precomp_id.has_value()) {
                auto it = registry.composition_id_map.find(*layer_spec.precomp_id);
                if (it != registry.composition_id_map.end()) {
                    compiled.graph.add_edge(it->second, layer.node.node_id, true);
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

    result.value = std::move(compiled);
    return result;
}

} // namespace tachyon

