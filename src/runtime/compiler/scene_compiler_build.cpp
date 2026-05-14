#include "tachyon/runtime/compiler/scene_compiler_detail.h"
#include "tachyon/runtime/compiler/scene_compiler_internal.h"
#include "tachyon/runtime/compiler/scene_compiler_hashing.h"
#include <algorithm>
#include <cstdint>

namespace tachyon::detail {

void build_compositions(const SceneSpec& scene, CompiledScene& compiled, tachyon::CompilationRegistry& registry, DiagnosticBag& diagnostics) {
    (void)diagnostics; // Not used yet, but kept for API compatibility
    
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
            // Resolve Source via variant
            std::visit([&](auto&& source) {
                using T = std::decay_t<decltype(source)>;
                if constexpr (std::is_same_v<T, MediaSource>) {
                    compiled_layer.asset_path = source.asset.id;
                } else if constexpr (std::is_same_v<T, ProceduralSource>) {
                    compiled_layer.procedural = source.spec;
                }
            }, layer.source);
            
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

            add_track(".opacity", layer.transform.opacity_property, layer.transform.opacity);
            add_track(".position_x", layer.transform.transform.position_property, layer.transform.transform.position_x.value_or(0.0));
            add_track(".position_y", layer.transform.transform.position_property, layer.transform.transform.position_y.value_or(0.0));
            add_track(".scale_x", layer.transform.transform.scale_property, layer.transform.transform.scale_x.value_or(100.0));
            add_track(".scale_y", layer.transform.transform.scale_property, layer.transform.transform.scale_y.value_or(100.0));
            add_track(".rotation", layer.transform.transform.rotation_property, layer.transform.transform.rotation.value_or(0.0));
            add_track(".masks.feather", layer.masks.feather, 0.0);
            
            // Anchor points
            add_track(".anchor_point_x", layer.transform.transform.anchor_point, 0.0);
            add_track(".anchor_point_y", layer.transform.transform.anchor_point, 0.0);

            // Populate Unified Fields (CompiledLayer has flat fields)
            compiled_layer.track_bindings = layer.playback.temporal.track_bindings;
            compiled_layer.time_remap = layer.playback.temporal.time_remap;
            compiled_layer.frame_blend = layer.playback.temporal.frame_blend;

            compiled_composition.layers.push_back(std::move(compiled_layer));
        }
        
        compiled_composition.audio_tracks = composition.audio_tracks;

        compiled.compositions.push_back(std::move(compiled_composition));
    }
}

} // namespace tachyon::detail
