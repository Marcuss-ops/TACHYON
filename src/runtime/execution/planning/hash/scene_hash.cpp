#include "src/runtime/execution/planning/hash/hash_utils.h"

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
            builder.add_string(*comp.background);
        }
        builder.add_bool(comp.environment_path.has_value());
        if (comp.environment_path.has_value()) {
            builder.add_string(*comp.environment_path);
        }
        builder.add_u64(static_cast<std::uint64_t>(comp.layers.size()));
        for (const auto& layer : comp.layers) {
            builder.add_string(layer.id);
            builder.add_string(layer.name);
            builder.add_string(layer.type);
            builder.add_string(layer.blend_mode);
            builder.add_bool(layer.enabled);
            builder.add_bool(layer.visible);
            builder.add_bool(layer.is_3d);
            builder.add_bool(layer.is_adjustment_layer);
            builder.add_bool(layer.motion_blur);
            builder.add_u64(static_cast<std::uint64_t>(layer.start_time * 1000.0));
            builder.add_u64(static_cast<std::uint64_t>(layer.in_point * 1000.0));
            builder.add_u64(static_cast<std::uint64_t>(layer.out_point * 1000.0));
            builder.add_u64(static_cast<std::uint64_t>(layer.opacity * 1000000.0));
            builder.add_u64(static_cast<std::uint64_t>(layer.width));
            builder.add_u64(static_cast<std::uint64_t>(layer.height));
            builder.add_bool(layer.parent.has_value());
            if (layer.parent.has_value()) builder.add_string(*layer.parent);
            builder.add_bool(layer.track_matte_layer_id.has_value());
            if (layer.track_matte_layer_id.has_value()) builder.add_string(*layer.track_matte_layer_id);
            builder.add_u32(static_cast<std::uint32_t>(layer.track_matte_type));
            builder.add_bool(layer.precomp_id.has_value());
            if (layer.precomp_id.has_value()) builder.add_string(*layer.precomp_id);
            builder.add_string(layer.text_content);
            builder.add_string(layer.font_id);
            builder.add_string(layer.alignment);
            builder.add_u64(static_cast<std::uint64_t>(layer.stroke_width * 1000000.0));
            builder.add_string(layer.subtitle_path);
            builder.add_string(layer.line_cap);
            builder.add_string(layer.line_join);
            builder.add_u64(static_cast<std::uint64_t>(layer.miter_limit * 1000.0));
            builder.add_string(layer.camera_type);
            builder.add_u64(static_cast<std::uint64_t>(layer.camera_shake_seed));
            builder.add_string(layer.light_type);
            builder.add_string(layer.falloff_type);
            builder.add_bool(layer.casts_shadows);
            hash::hash_animated_scalar(builder, layer.opacity_property);
            hash::hash_animated_scalar(builder, layer.mask_feather);
            hash::hash_animated_scalar(builder, layer.time_remap_property);
            hash::hash_animated_scalar(builder, layer.font_size);
            hash::hash_animated_scalar(builder, layer.stroke_width_property);
            hash::hash_animated_scalar(builder, layer.repeater_count);
            hash::hash_animated_scalar(builder, layer.repeater_stagger_delay);
            hash::hash_animated_scalar(builder, layer.repeater_offset_position_x);
            hash::hash_animated_scalar(builder, layer.repeater_offset_position_y);
            hash::hash_animated_scalar(builder, layer.repeater_offset_rotation);
            hash::hash_animated_scalar(builder, layer.repeater_offset_scale_x);
            hash::hash_animated_scalar(builder, layer.repeater_offset_scale_y);
            hash::hash_animated_scalar(builder, layer.repeater_start_opacity);
            hash::hash_animated_scalar(builder, layer.repeater_end_opacity);
            hash::hash_animated_scalar(builder, layer.light_intensity);
            hash::hash_animated_scalar(builder, layer.attenuation_near);
            hash::hash_animated_scalar(builder, layer.attenuation_far);
            hash::hash_animated_scalar(builder, layer.cone_angle);
            hash::hash_animated_scalar(builder, layer.cone_feather);
            hash::hash_animated_scalar(builder, layer.shadow_darkness);
            hash::hash_animated_scalar(builder, layer.shadow_radius);
            hash::hash_animated_scalar(builder, layer.camera_zoom);
            hash::hash_animated_vector3(builder, layer.camera_poi);
            hash::hash_animated_scalar(builder, layer.camera_shake_amplitude_pos);
            hash::hash_animated_scalar(builder, layer.camera_shake_amplitude_rot);
            hash::hash_animated_scalar(builder, layer.camera_shake_frequency);
            hash::hash_animated_scalar(builder, layer.camera_shake_roughness);
            hash::hash_animated_color(builder, layer.fill_color);
            hash::hash_animated_color(builder, layer.stroke_color);
            hash::hash_animated_color(builder, layer.light_color);
            builder.add_u64(static_cast<std::uint64_t>(layer.effects.size()));
            for (const auto& effect : layer.effects) hash::hash_effect(builder, effect);
            builder.add_u64(static_cast<std::uint64_t>(layer.text_animators.size()));
            for (const auto& animator : layer.text_animators) hash::hash_text_animator(builder, animator);
            builder.add_u64(static_cast<std::uint64_t>(layer.text_highlights.size()));
            for (const auto& highlight : layer.text_highlights) hash::hash_text_highlight(builder, highlight);
            builder.add_u64(static_cast<std::uint64_t>(layer.track_bindings.size()));
            for (const auto& binding : layer.track_bindings) {
                builder.add_string(binding.property_path);
                builder.add_string(binding.source_id);
                builder.add_string(binding.source_track_name);
                builder.add_f64(binding.influence);
                builder.add_bool(binding.enabled);
            }
            builder.add_u64(static_cast<std::uint64_t>(layer.mask_paths.size()));
            for (const auto& mask : layer.mask_paths) {
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
            builder.add_bool(layer.time_remap.enabled);
            builder.add_u32(static_cast<std::uint32_t>(layer.time_remap.mode));
            builder.add_u64(static_cast<std::uint64_t>(layer.time_remap.keyframes.size()));
            for (const auto& kf : layer.time_remap.keyframes) {
                builder.add_f64(kf.first);
                builder.add_f64(kf.second);
            }
            builder.add_u32(static_cast<std::uint32_t>(layer.frame_blend));
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
