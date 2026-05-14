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
            
            compiled_layer.type_id = compiled_type_id_from_layer_type(layer.type);
            
            compiled_layer.width = static_cast<std::uint32_t>(layer.width);
            compiled_layer.height = static_cast<std::uint32_t>(layer.height);
            compiled_layer.text.content = layer.text.content;
            compiled_layer.text.font_id = layer.text.text.font_id;
            compiled_layer.text.font_size = static_cast<float>(layer.text.text.font_size.value.has_value() ? *layer.text.text.font_size.value : 48.0);
            compiled_layer.text.box = layer.text.box;
            compiled_layer.text.fill_color = layer.text.text.fill_color.value.has_value() ? *layer.text.text.fill_color.value : ColorSpec{255, 255, 255, 255};
            compiled_layer.text.stroke_color = layer.text.text.stroke_color.value.has_value() ? *layer.text.text.stroke_color.value : ColorSpec{255, 255, 255, 255};
            compiled_layer.text.stroke_width = layer.text.text.text.stroke_width_property.value.has_value() ? static_cast<float>(*layer.text.text.text.stroke_width_property.value) : static_cast<float>(layer.text.text.stroke_width);
            compiled_layer.text_animators = layer.text_animators;
            compiled_layer.text_highlights = layer.text_highlights;
            
            compiled_layer.vector.shape_path = layer.vector.vector.shape_path;
            compiled_layer.effects = layer.effects;
            compiled_layer.procedural = layer.procedural;
            
            compiled_layer.masks.feather = static_cast<float>(layer.masks.feather.value.has_value() ? *layer.masks.feather.value : 0.0);
            compiled_layer.subtitles.path = layer.subtitles.path;
            compiled_layer.subtitles.outline_color = layer.subtitles.outline_color;
            compiled_layer.subtitles.outline_width = static_cast<float>(layer.subtitles.outline_width);
            compiled_layer.subtitles.word_timestamp_path = layer.subtitles.subtitles.word_timestamp_path;
            
            if (layer.vector.vector.line_cap == "round") compiled_layer.vector.line_cap = renderer2d::LineCap::Round;
            else if (layer.vector.vector.line_cap == "square") compiled_layer.vector.line_cap = renderer2d::LineCap::Square;
            else compiled_layer.vector.line_cap = renderer2d::LineCap::Butt;

            if (layer.vector.vector.line_join == "round") compiled_layer.vector.line_join = renderer2d::LineJoin::Round;
            else if (layer.vector.vector.line_join == "bevel") compiled_layer.vector.line_join = renderer2d::LineJoin::Bevel;
            else compiled_layer.vector.line_join = renderer2d::LineJoin::Miter;

            compiled_layer.vector.miter_limit = static_cast<float>(layer.vector.vector.miter_limit);
            
            // Build flags bitmask
            compiled_layer.flags = 0;
            if (layer.enabled) compiled_layer.flags |= 0x01;
            if (layer.visible) compiled_layer.flags |= 0x02;
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
            add_track(".masks.feather", layer.masks.feather, 0.0);
            
            // Anchor points
            add_track(".anchor_point_x", layer.transform.anchor_point, layer.transform.anchor_point.value.has_value() ? layer.transform.anchor_point.value->x : 0.0);
            add_track(".anchor_point_y", layer.transform.anchor_point, layer.transform.anchor_point.value.has_value() ? layer.transform.anchor_point.value->y : 0.0);

            // Populate Unified Fields
            compiled_layer.temporal.track_bindings = layer.temporal.track_bindings;
            compiled_layer.temporal.time_remap = layer.temporal.time_remap;
            compiled_layer.temporal.frame_blend = layer.temporal.frame_blend;

            compiled_composition.layers.push_back(std::move(compiled_layer));
        }
        
        compiled_composition.audio_tracks = composition.audio_tracks;

        compiled.compositions.push_back(std::move(compiled_composition));
    }
}

} // namespace tachyon::detail
