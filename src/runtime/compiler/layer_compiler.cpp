#include "layer_compiler.h"
#include "tachyon/runtime/compiler/property_compiler.h"
#include <algorithm>

namespace tachyon {

CompiledLayer LayerCompiler::compile_layer(
    CompilationRegistry& registry,
    const LayerSpec& layer,
    const std::string& layer_path,
    const SceneSpec& scene,
    CompiledScene& compiled,
    DiagnosticBag& diagnostics) {

    CompiledLayer compiled_layer;
    compiled_layer.node = registry.create_node(CompiledNodeType::Layer);
    compiled.graph.add_node(compiled_layer.node.node_id);

    // Resolve Type
    compiled_layer.type_id = compiled_type_id_from_layer_type(layer.identity.type);
    if (compiled_layer.type_id == 0 && layer.identity.type != LayerType::Unknown) {
        diagnostics.add_warning("COMPILER_W001",
            "Layer '" + layer.identity.id + "' has unknown type '" + std::string(to_canonical_layer_type_string(layer.identity.type)) + "', will render as null layer.",
            layer_path);
    }

    // Asset resolution check for image/video layers
    if (compiled_layer.type_id == 3) { // Image/Video
        const bool found_in_assets = std::any_of(scene.assets.begin(), scene.assets.end(),
            [&](const AssetSpec& a) { return a.id == layer.source.asset_path || a.id == layer.identity.name; });
        if (!found_in_assets) {
            compiled_layer.asset_offline = true;
            diagnostics.add_warning("COMPILER_W004",
                "Image layer '" + layer.identity.id + "' has no matching asset in scene.assets — will render as transparent.",
                layer_path);
        }
    }
    
    compiled_layer.width = static_cast<std::uint32_t>(layer.transform.width);
    compiled_layer.height = static_cast<std::uint32_t>(layer.transform.height);
    compiled_layer.text.content = layer.text.content;
    compiled_layer.text.font_id = layer.text.font_id;
    compiled_layer.text.font_size = static_cast<float>(layer.text.font_size.value.has_value() ? *layer.text.font_size.value : 48.0);
    compiled_layer.text.box = layer.text.box;
    compiled_layer.text.fill_color = layer.text.fill_color.value.has_value() ? *layer.text.fill_color.value : ColorSpec{255, 255, 255, 255};
    compiled_layer.text.stroke_color = layer.text.stroke_color.value.has_value() ? *layer.text.stroke_color.value : ColorSpec{255, 255, 255, 255};
    compiled_layer.text.stroke_width = layer.text.stroke_width_property.value.has_value() ? static_cast<float>(*layer.text.stroke_width_property.value) : static_cast<float>(layer.text.stroke_width);
    compiled_layer.vector.shape_path = layer.vector.shape_path;
    compiled_layer.vector.shape_spec = layer.vector.shape_spec;
    compiled_layer.effects = layer.effects;
    compiled_layer.text_animators = layer.text_animators;
    compiled_layer.text_highlights = layer.text_highlights;

    compiled_layer.masks.feather = static_cast<float>(layer.masks.feather.value.has_value() ? *layer.masks.feather.value : 0.0);
    compiled_layer.subtitles.path = layer.subtitles.path;
    compiled_layer.subtitles.outline_color = layer.subtitles.outline_color;
    compiled_layer.subtitles.outline_width = static_cast<float>(layer.subtitles.outline_width);
    compiled_layer.subtitles.word_timestamp_path = layer.subtitles.word_timestamp_path;
    compiled_layer.procedural = layer.source.procedural;
    
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

    // Resolve Property Indices
    const auto add_track = [&](const std::string& suffix, const auto& spec, double fallback) {
        compiled_layer.property_indices.push_back(static_cast<std::uint32_t>(compiled.property_tracks.size()));
        auto track = PropertyCompiler::compile_track(registry, suffix, layer.identity.id, spec, fallback, compiled.expressions, diagnostics);
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

    // Additional canonical tracks
    add_track(".playback.temporal.time_remap", layer.playback.temporal.time_remap_property, 0.0);
    add_track(".text.font_size", layer.text.font_size, 48.0);
    add_track(".text.stroke_width", layer.text.stroke_width_property, layer.text.stroke_width);
    add_track(".repeater.count", layer.repeater.count, 1.0);
    add_track(".repeater.stagger_delay", layer.repeater.stagger_delay, 0.0);

    // Populate Unified Fields
    compiled_layer.track_bindings = layer.playback.temporal.track_bindings;
    compiled_layer.time_remap = layer.playback.temporal.time_remap;
    compiled_layer.frame_blend = layer.playback.temporal.frame_blend;

    compiled_layer.in_time = layer.playback.timing.start;
    compiled_layer.out_time = layer.playback.timing.start + layer.playback.timing.duration;
    compiled_layer.start_time = layer.playback.timing.start;
    compiled_layer.blend_mode = layer.blend_mode;
    compiled_layer.transition_in = layer.transition_in;
    compiled_layer.transition_out = layer.transition_out;

    return compiled_layer;
}

} // namespace tachyon
