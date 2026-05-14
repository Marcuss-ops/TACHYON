#include "hash_utils.h"

namespace tachyon::hash {

void hash_transition(CacheKeyBuilder& builder, const LayerTransitionSpec& spec) {
    builder.add_string(spec.transition_id);
    builder.add_u32(static_cast<std::uint32_t>(spec.direction));
    builder.add_f64(spec.duration);
    builder.add_f64(spec.delay);
    builder.add_u32(static_cast<std::uint32_t>(spec.easing));
    builder.add_f64(spec.spring.stiffness);
    builder.add_f64(spec.spring.damping);
    builder.add_f64(spec.spring.mass);
    builder.add_f64(spec.spring.velocity);
}

void hash_procedural(CacheKeyBuilder& builder, const ProceduralSpec& spec) {
    builder.add_string(spec.kind);
    builder.add_u64(spec.seed);
    hash_animated_scalar(builder, spec.frequency);
    hash_animated_scalar(builder, spec.speed);
    hash_animated_scalar(builder, spec.amplitude);
    hash_animated_scalar(builder, spec.scale);
    hash_animated_scalar(builder, spec.angle);
    hash_animated_color(builder, spec.color_a);
    hash_animated_color(builder, spec.color_b);
    hash_animated_color(builder, spec.color_c);
    builder.add_string(spec.shape);
    hash_animated_scalar(builder, spec.spacing);
    hash_animated_scalar(builder, spec.border_width);
    hash_animated_color(builder, spec.border_color);
    hash_animated_scalar(builder, spec.warp_strength);
    hash_animated_scalar(builder, spec.warp_frequency);
    hash_animated_scalar(builder, spec.warp_speed);
    hash_animated_scalar(builder, spec.grain_amount);
    hash_animated_scalar(builder, spec.grain_scale);
    hash_animated_scalar(builder, spec.scanline_intensity);
    hash_animated_scalar(builder, spec.scanline_frequency);
    hash_animated_scalar(builder, spec.contrast);
    hash_animated_scalar(builder, spec.gamma);
    hash_animated_scalar(builder, spec.saturation);
    hash_animated_scalar(builder, spec.softness);
    hash_animated_scalar(builder, spec.glow_intensity);
    hash_animated_scalar(builder, spec.vignette_intensity);
    hash_animated_scalar(builder, spec.fisheye_strength);
    hash_animated_scalar(builder, spec.ripple_intensity);
    hash_animated_scalar(builder, spec.mouse_influence);
    hash_animated_scalar(builder, spec.mouse_radius);
    builder.add_f32(spec.mouse_x);
    builder.add_f32(spec.mouse_y);
    hash_animated_scalar(builder, spec.octave_decay);
    hash_animated_scalar(builder, spec.band_height);
    hash_animated_scalar(builder, spec.band_spread);
    hash_animated_scalar(builder, spec.star_speed);
    hash_animated_scalar(builder, spec.density);
    hash_animated_scalar(builder, spec.hue_shift);
    hash_animated_scalar(builder, spec.twinkle_intensity);
    hash_animated_scalar(builder, spec.rotation_speed);
    hash_animated_scalar(builder, spec.repulsion_strength);
    hash_animated_scalar(builder, spec.auto_center_repulsion);
    builder.add_f32(spec.focal_x);
    builder.add_f32(spec.focal_y);
    builder.add_bool(spec.transparent);
}

void hash_shape(CacheKeyBuilder& builder, const ShapeSpec& spec) {
    builder.add_string(spec.type);
    builder.add_f32(spec.x);
    builder.add_f32(spec.y);
    builder.add_f32(spec.width);
    builder.add_f32(spec.height);
    builder.add_f32(spec.radius);
    builder.add_u32(static_cast<std::uint32_t>(spec.sides));
    builder.add_f32(spec.inner_radius);
    builder.add_f32(spec.outer_radius);
    builder.add_f32(spec.x1);
    builder.add_f32(spec.y1);
    builder.add_f32(spec.head_size);
    builder.add_f32(spec.tail_x);
    builder.add_f32(spec.tail_y);
    builder.add_f32(spec.stroke_width);
    builder.add_u32(static_cast<std::uint32_t>(spec.line_cap));
    builder.add_u32(static_cast<std::uint32_t>(spec.line_join));
    builder.add_u64(static_cast<std::uint64_t>(spec.dash_array.size()));
    for (float d : spec.dash_array) builder.add_f32(d);
    builder.add_f32(spec.dash_offset);
    builder.add_f32(spec.sdf_threshold);
    builder.add_u32(spec.fill_color.r);
    builder.add_u32(spec.fill_color.g);
    builder.add_u32(spec.fill_color.b);
    builder.add_u32(spec.fill_color.a);
    builder.add_u32(spec.stroke_color.r);
    builder.add_u32(spec.stroke_color.g);
    builder.add_u32(spec.stroke_color.b);
    builder.add_u32(spec.stroke_color.a);
    builder.add_f32(spec.opacity);
}

} // namespace tachyon::hash

namespace tachyon {

std::uint64_t hash_scene_content(const SceneSpec& scene) {
    CacheKeyBuilder builder;
    builder.add_string(scene.project.id);
    builder.add_string(scene.project.name);
    builder.add_string(scene.project.authoring_tool);
    builder.add_bool(scene.project.root_seed.has_value());
    if (scene.project.root_seed.has_value()) {
        builder.add_u64(static_cast<std::uint64_t>(*scene.project.root_seed));
    }
    builder.add_u64(static_cast<std::uint64_t>(scene.compositions.size()));
    for (const auto& comp : scene.compositions) {
        builder.add_string(comp.id);
        builder.add_string(comp.name);
        builder.add_u64(static_cast<std::uint64_t>(comp.width));
        builder.add_u64(static_cast<std::uint64_t>(comp.height));
        builder.add_u64(static_cast<std::uint64_t>(comp.duration * 1000.0));
        builder.add_u64(static_cast<std::uint64_t>(comp.frame_rate.numerator));
        builder.add_u64(static_cast<std::uint64_t>(comp.frame_rate.denominator));
        builder.add_bool(comp.fps.has_value());
        if (comp.fps.has_value()) {
            builder.add_u64(static_cast<std::uint64_t>(*comp.fps));
        }
        builder.add_bool(comp.background.has_value());
        if (comp.background.has_value()) {
            builder.add_u32(static_cast<std::uint32_t>(comp.background->type));
            builder.add_string(comp.background->value);
            if (comp.background->parsed_color.has_value()) {
                const auto& c = *comp.background->parsed_color;
                builder.add_u32(c.r);
                builder.add_u32(c.g);
                builder.add_u32(c.b);
                builder.add_u32(c.a);
            }
        }
        builder.add_u64(static_cast<std::uint64_t>(comp.layers.size()));
        for (const auto& layer : comp.layers) {
            builder.add_string(layer.identity.id);
            builder.add_string(layer.identity.name);
            builder.add_string(to_canonical_layer_type_string(layer.identity.type));
            builder.add_u32(static_cast<std::uint32_t>(layer.blend_mode));
            builder.add_bool(layer.identity.enabled);
            builder.add_bool(layer.identity.visible);
            builder.add_bool(layer.identity.is_adjustment_layer);
            builder.add_bool(layer.identity.motion_blur);
            
            builder.add_f64(layer.playback.timing.start);
            builder.add_f64(layer.playback.timing.duration);
            builder.add_f64(layer.playback.timing.source_in);
            builder.add_f64(layer.playback.timing.source_out);

            builder.add_u64(static_cast<std::uint64_t>(layer.transform.opacity * 1000000.0));
            builder.add_u64(static_cast<std::uint64_t>(layer.transform.width));
            builder.add_u64(static_cast<std::uint64_t>(layer.transform.height));
            
            if (layer.transform.transform.position_x.has_value()) builder.add_f64(*layer.transform.transform.position_x);
            if (layer.transform.transform.position_y.has_value()) builder.add_f64(*layer.transform.transform.position_y);
            if (layer.transform.transform.rotation.has_value()) builder.add_f64(*layer.transform.transform.rotation);
            if (layer.transform.transform.scale_x.has_value()) builder.add_f64(*layer.transform.transform.scale_x);
            if (layer.transform.transform.scale_y.has_value()) builder.add_f64(*layer.transform.transform.scale_y);
            hash::hash_animated_vector2(builder, layer.transform.transform.position_property);
            hash::hash_animated_vector2(builder, layer.transform.transform.anchor_point);
            hash::hash_animated_scalar(builder, layer.transform.transform.rotation_property);
            hash::hash_animated_vector2(builder, layer.transform.transform.scale_property);

            builder.add_bool(layer.parent.has_value());
            if (layer.parent.has_value()) builder.add_string(*layer.parent);
            builder.add_bool(layer.track_matte_layer_id.has_value());
            if (layer.track_matte_layer_id.has_value()) builder.add_string(*layer.track_matte_layer_id);
            builder.add_u32(static_cast<std::uint32_t>(layer.track_matte_type));
            builder.add_bool(layer.source.precomp_id.has_value());
            if (layer.source.precomp_id.has_value()) builder.add_string(*layer.source.precomp_id);
            builder.add_string(layer.text.content);
            builder.add_string(layer.text.font_id);
            builder.add_u32(static_cast<std::uint32_t>(layer.text.box.horizontal_align));
            builder.add_u32(static_cast<std::uint32_t>(layer.text.box.vertical_align));
            builder.add_bool(layer.text.box.fixed_pitch);
            builder.add_u64(static_cast<std::uint64_t>(layer.text.stroke_width * 1000000.0));
            builder.add_string(layer.subtitles.path);
            builder.add_u32(static_cast<std::uint32_t>(layer.vector.line_cap));
            builder.add_u32(static_cast<std::uint32_t>(layer.vector.line_join));
            builder.add_u64(static_cast<std::uint64_t>(layer.vector.miter_limit * 1000.0));
            builder.add_bool(layer.transform.camera2d_id.has_value());
            if (layer.transform.camera2d_id.has_value()) builder.add_string(*layer.transform.camera2d_id);
            
            builder.add_f64(layer.transform.parallax_factor);
            builder.add_bool(layer.transform.has_parallax);
            
            hash::hash_animated_color(builder, layer.subtitles.outline_color);
            builder.add_f64(layer.subtitles.outline_width);
            
            builder.add_string(layer.animation_in_preset);
            builder.add_string(layer.animation_during_preset);
            builder.add_string(layer.animation_out_preset);
            builder.add_bool(layer.playback.loop);
            builder.add_bool(layer.playback.hold_last_frame);
            
            hash::hash_transition(builder, layer.transition_in);
            hash::hash_transition(builder, layer.transition_out);

            hash::hash_animated_scalar(builder, layer.transform.opacity_property);
            hash::hash_animated_scalar(builder, layer.masks.feather);
            hash::hash_animated_scalar(builder, layer.playback.temporal.time_remap_property);
            hash::hash_animated_scalar(builder, layer.text.font_size);
            hash::hash_animated_scalar(builder, layer.text.stroke_width_property);
            hash::hash_animated_scalar(builder, layer.repeater.count);
            hash::hash_animated_scalar(builder, layer.repeater.stagger_delay);
            hash::hash_animated_scalar(builder, layer.repeater.offset_position_x);
            hash::hash_animated_scalar(builder, layer.repeater.offset_position_y);
            hash::hash_animated_scalar(builder, layer.repeater.offset_rotation);
            hash::hash_animated_scalar(builder, layer.repeater.offset_scale_x);
            hash::hash_animated_scalar(builder, layer.repeater.offset_scale_y);
            hash::hash_animated_scalar(builder, layer.repeater.start_opacity);
            hash::hash_animated_scalar(builder, layer.repeater.end_opacity);
            
            hash::hash_animated_scalar(builder, layer.repeater.grid_cols);
            hash::hash_animated_scalar(builder, layer.repeater.radial_radius);
            hash::hash_animated_scalar(builder, layer.repeater.radial_start_angle);
            hash::hash_animated_scalar(builder, layer.repeater.radial_end_angle);

            hash::hash_animated_color(builder, layer.text.fill_color);
            hash::hash_animated_color(builder, layer.text.stroke_color);
            builder.add_u64(static_cast<std::uint64_t>(layer.effects.size()));
            for (const auto& effect : layer.effects) hash::hash_effect(builder, effect);
            
            if (layer.source.procedural.has_value()) {
                builder.add_bool(true);
                hash::hash_procedural(builder, *layer.source.procedural);
            } else {
                builder.add_bool(false);
            }

            if (layer.vector.shape_spec.has_value()) {
                builder.add_bool(true);
                hash::hash_shape(builder, *layer.vector.shape_spec);
            } else {
                builder.add_bool(false);
            }

            builder.add_u64(static_cast<std::uint64_t>(layer.playback.markers.size()));
            for (const auto& marker : layer.playback.markers) {
                builder.add_f64(marker.time);
                builder.add_string(marker.label);
                builder.add_string(marker.color);
            }

            builder.add_u64(static_cast<std::uint64_t>(layer.text_animators.size()));
            for (const auto& animator : layer.text_animators) hash::hash_text_animator(builder, animator);
            builder.add_u64(static_cast<std::uint64_t>(layer.text_highlights.size()));
            for (const auto& highlight : layer.text_highlights) hash::hash_text_highlight(builder, highlight);
            builder.add_u64(static_cast<std::uint64_t>(layer.playback.temporal.track_bindings.size()));
            for (const auto& binding : layer.playback.temporal.track_bindings) {
                builder.add_string(binding.property_path);
                builder.add_string(binding.source_id);
                builder.add_string(binding.source_track_name);
                builder.add_f64(binding.influence);
                builder.add_bool(binding.enabled);
            }
            builder.add_u64(static_cast<std::uint64_t>(layer.masks.paths.size()));
            for (const auto& mask : layer.masks.paths) {
                builder.add_u64(static_cast<std::uint64_t>(mask.vertices.size()));
                builder.add_bool(mask.is_closed);
                builder.add_bool(mask.is_inverted);
                for (const auto& vertex : mask.vertices) {
                    builder.add_f64(vertex.position.x);
                    builder.add_f64(vertex.position.y);
                    builder.add_f64(vertex.in_tangent.x);
                    builder.add_f64(vertex.in_tangent.y);
                    builder.add_f64(vertex.out_tangent.x);
                    builder.add_f64(vertex.out_tangent.y);
                    builder.add_f64(vertex.feather_inner);
                    builder.add_f64(vertex.feather_outer);
                }
            }
            builder.add_bool(layer.playback.temporal.time_remap.enabled);
            builder.add_u32(static_cast<std::uint32_t>(layer.playback.temporal.time_remap.mode));
            builder.add_u64(static_cast<std::uint64_t>(layer.playback.temporal.time_remap.keyframes.size()));
            for (const auto& kf : layer.playback.temporal.time_remap.keyframes) {
                builder.add_f64(kf.first);
                builder.add_f64(kf.second);
            }
            builder.add_u32(static_cast<std::uint32_t>(layer.playback.temporal.frame_blend));
        }
    }
    builder.add_u64(static_cast<std::uint64_t>(scene.assets.size()));
    for (const auto& asset : scene.assets) {
        builder.add_string(asset.id);
        builder.add_string(asset.type);
        builder.add_string(asset.path);
        builder.add_string(asset.source);
        builder.add_bool(asset.alpha_mode.has_value());
        if (asset.alpha_mode.has_value()) {
            builder.add_string(*asset.alpha_mode);
        }
    }
    builder.add_u64(static_cast<std::uint64_t>(scene.data_sources.size()));
    for (const auto& ds : scene.data_sources) {
        builder.add_string(ds.id);
        builder.add_string(ds.path);
        builder.add_string(ds.type);
    }
    return builder.finish();
}

} // namespace tachyon
