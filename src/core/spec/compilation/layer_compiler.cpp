#include "layer_compiler.h"
#include "tachyon/core/spec/compilation/property_compiler.h"
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
    compiled_layer.type_id = compiled_type_id_from_layer_type(layer.type);
    if (compiled_layer.type_id == 0 && layer.type != LayerType::Unknown) {
        diagnostics.add_warning("COMPILER_W001",
            "Layer '" + layer.id + "' has unknown type '" + std::string(to_canonical_layer_type_string(layer.type)) + "', will render as null layer.",
            layer_path);
    }

    // Asset resolution check for image/video layers
    if (compiled_layer.type_id == 3) {
        const bool found_in_assets = std::any_of(scene.assets.begin(), scene.assets.end(),
            [&](const AssetSpec& a) { return a.id == layer.id || a.id == layer.name; });
        if (!found_in_assets) {
            compiled_layer.asset_offline = true;
            diagnostics.add_warning("COMPILER_W004",
                "Image layer '" + layer.id + "' has no matching asset in scene.assets — will render as transparent.",
                layer_path);
        }
    }
    
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
    compiled_layer.procedural = layer.procedural;
    
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

    // Resolve Property Indices
    const auto add_track = [&](const std::string& suffix, const auto& spec, double fallback) {
        compiled_layer.property_indices.push_back(static_cast<std::uint32_t>(compiled.property_tracks.size()));
        auto track = PropertyCompiler::compile_track(registry, suffix, layer.id, spec, fallback, compiled.expressions, diagnostics);
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
    add_track(".anchor_point_x", layer.transform.anchor_point, layer.transform.anchor_point.value.has_value() ? layer.transform.anchor_point.value->x : 0.0);
    add_track(".anchor_point_y", layer.transform.anchor_point, layer.transform.anchor_point.value.has_value() ? layer.transform.anchor_point.value->y : 0.0);

    // Populate Unified Fields
    compiled_layer.track_bindings = layer.track_bindings;
    compiled_layer.time_remap = layer.time_remap;
    compiled_layer.frame_blend = layer.frame_blend;

    compiled_layer.in_time = layer.in_point;
    compiled_layer.out_time = layer.out_point;
    compiled_layer.start_time = layer.start_time;
    compiled_layer.blend_mode = layer.blend_mode;
    compiled_layer.transition_in = layer.transition_in;
    compiled_layer.transition_out = layer.transition_out;

    return compiled_layer;
}

} // namespace tachyon
