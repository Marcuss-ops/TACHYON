#include "tachyon/runtime/execution/planning/render_plan.h"
#include "tachyon/runtime/cache/cache_key_builder.h"

#include <algorithm>
#include <string_view>

namespace tachyon {
namespace {

const CompositionSpec* find_composition(const SceneSpec& scene, const std::string& composition_id) {
    const auto it = std::find_if(scene.compositions.begin(), scene.compositions.end(), [&](const CompositionSpec& composition) {
        return composition.id == composition_id;
    });
    if (it == scene.compositions.end()) {
        return nullptr;
    }
    return &(*it);
}

std::size_t count_layers_with_type(const CompositionSpec& composition, const std::string& type) {
    return static_cast<std::size_t>(std::count_if(
        composition.layers.begin(),
        composition.layers.end(),
        [&](const LayerSpec& layer) { return layer.type == type; }));
}

std::size_t count_layers_with_track_matte(const CompositionSpec& composition) {
    return static_cast<std::size_t>(std::count_if(
        composition.layers.begin(),
        composition.layers.end(),
        [&](const LayerSpec& layer) { return layer.track_matte_type != TrackMatteType::None; }));
}

template <typename T>
void hash_keyframes(CacheKeyBuilder& builder, const std::vector<T>& keyframes) {
    builder.add_u64(static_cast<std::uint64_t>(keyframes.size()));
    for (const auto& kf : keyframes) {
        builder.add_f64(kf.time);
        builder.add_u64(static_cast<std::uint64_t>(kf.easing));
        builder.add_u64(static_cast<std::uint64_t>(kf.bezier.cx1 * 1000000.0));
        builder.add_u64(static_cast<std::uint64_t>(kf.bezier.cy1 * 1000000.0));
        builder.add_u64(static_cast<std::uint64_t>(kf.bezier.cx2 * 1000000.0));
        builder.add_u64(static_cast<std::uint64_t>(kf.bezier.cy2 * 1000000.0));
        builder.add_f64(kf.speed_in);
        builder.add_f64(kf.influence_in);
        builder.add_f64(kf.speed_out);
        builder.add_f64(kf.influence_out);
    }
}

void hash_animated_scalar(CacheKeyBuilder& builder, const AnimatedScalarSpec& spec) {
    builder.add_bool(spec.value.has_value());
    if (spec.value.has_value()) {
        builder.add_f64(*spec.value);
    }
    builder.add_u64(static_cast<std::uint64_t>(spec.keyframes.size()));
    for (const auto& kf : spec.keyframes) {
        builder.add_f64(kf.time);
        builder.add_f64(kf.value);
        builder.add_u64(static_cast<std::uint64_t>(kf.easing));
        builder.add_u64(static_cast<std::uint64_t>(kf.bezier.cx1 * 1000000.0));
        builder.add_u64(static_cast<std::uint64_t>(kf.bezier.cy1 * 1000000.0));
        builder.add_u64(static_cast<std::uint64_t>(kf.bezier.cx2 * 1000000.0));
        builder.add_u64(static_cast<std::uint64_t>(kf.bezier.cy2 * 1000000.0));
        builder.add_f64(kf.speed_in);
        builder.add_f64(kf.influence_in);
        builder.add_f64(kf.speed_out);
        builder.add_f64(kf.influence_out);
    }
    builder.add_bool(spec.audio_band.has_value());
    if (spec.audio_band.has_value()) {
        builder.add_u32(static_cast<std::uint32_t>(*spec.audio_band));
    }
    builder.add_f64(spec.audio_min);
    builder.add_f64(spec.audio_max);
    builder.add_bool(spec.expression.has_value());
    if (spec.expression.has_value()) {
        builder.add_string(*spec.expression);
    }
}

void hash_animated_vector2(CacheKeyBuilder& builder, const AnimatedVector2Spec& spec) {
    builder.add_bool(spec.value.has_value());
    if (spec.value.has_value()) {
        builder.add_f64(spec.value->x);
        builder.add_f64(spec.value->y);
    }
    builder.add_u64(static_cast<std::uint64_t>(spec.keyframes.size()));
    for (const auto& kf : spec.keyframes) {
        builder.add_f64(kf.time);
        builder.add_f64(kf.value.x);
        builder.add_f64(kf.value.y);
        builder.add_f64(kf.tangent_in.x);
        builder.add_f64(kf.tangent_in.y);
        builder.add_f64(kf.tangent_out.x);
        builder.add_f64(kf.tangent_out.y);
        builder.add_u32(static_cast<std::uint32_t>(kf.easing));
        builder.add_u64(static_cast<std::uint64_t>(kf.bezier.cx1 * 1000000.0));
        builder.add_u64(static_cast<std::uint64_t>(kf.bezier.cy1 * 1000000.0));
        builder.add_u64(static_cast<std::uint64_t>(kf.bezier.cx2 * 1000000.0));
        builder.add_u64(static_cast<std::uint64_t>(kf.bezier.cy2 * 1000000.0));
        builder.add_f64(kf.speed_in);
        builder.add_f64(kf.influence_in);
        builder.add_f64(kf.speed_out);
        builder.add_f64(kf.influence_out);
    }
    builder.add_bool(spec.expression.has_value());
    if (spec.expression.has_value()) {
        builder.add_string(*spec.expression);
    }
}

void hash_effect(CacheKeyBuilder& builder, const EffectSpec& effect) {
    builder.add_string(effect.type);
    builder.add_bool(effect.enabled);
    // Hash scalars
    builder.add_u64(static_cast<std::uint64_t>(effect.scalars.size()));
    for (const auto& [key, val] : effect.scalars) {
        builder.add_string(key);
        builder.add_f64(val);
    }
    // Hash colors
    builder.add_u64(static_cast<std::uint64_t>(effect.colors.size()));
    for (const auto& [key, val] : effect.colors) {
        builder.add_string(key);
        builder.add_u64(val.r);
        builder.add_u64(val.g);
        builder.add_u64(val.b);
        builder.add_u64(val.a);
    }
    // Hash strings
    builder.add_u64(static_cast<std::uint64_t>(effect.strings.size()));
    for (const auto& [key, val] : effect.strings) {
        builder.add_string(key);
        builder.add_string(val);
    }
}

void hash_animated_vector3(CacheKeyBuilder& builder, const AnimatedVector3Spec& spec) {
    builder.add_bool(spec.value.has_value());
    if (spec.value.has_value()) {
        builder.add_f64(spec.value->x);
        builder.add_f64(spec.value->y);
        builder.add_f64(spec.value->z);
    }
    builder.add_u64(static_cast<std::uint64_t>(spec.keyframes.size()));
    for (const auto& kf : spec.keyframes) {
        builder.add_f64(kf.time);
        builder.add_f64(kf.value.x);
        builder.add_f64(kf.value.y);
        builder.add_f64(kf.value.z);
        builder.add_f64(kf.tangent_in.x);
        builder.add_f64(kf.tangent_in.y);
        builder.add_f64(kf.tangent_in.z);
        builder.add_f64(kf.tangent_out.x);
        builder.add_f64(kf.tangent_out.y);
        builder.add_f64(kf.tangent_out.z);
        builder.add_u32(static_cast<std::uint32_t>(kf.easing));
        builder.add_u64(static_cast<std::uint64_t>(kf.bezier.cx1 * 1000000.0));
        builder.add_u64(static_cast<std::uint64_t>(kf.bezier.cy1 * 1000000.0));
        builder.add_u64(static_cast<std::uint64_t>(kf.bezier.cx2 * 1000000.0));
        builder.add_u64(static_cast<std::uint64_t>(kf.bezier.cy2 * 1000000.0));
        builder.add_f64(kf.speed_in);
        builder.add_f64(kf.influence_in);
        builder.add_f64(kf.speed_out);
        builder.add_f64(kf.influence_out);
    }
    builder.add_bool(spec.expression.has_value());
    if (spec.expression.has_value()) {
        builder.add_string(*spec.expression);
    }
}

void hash_animated_color(CacheKeyBuilder& builder, const AnimatedColorSpec& spec) {
    builder.add_bool(spec.value.has_value());
    if (spec.value.has_value()) {
        builder.add_u32(spec.value->r);
        builder.add_u32(spec.value->g);
        builder.add_u32(spec.value->b);
        builder.add_u32(spec.value->a);
    }
    builder.add_u64(static_cast<std::uint64_t>(spec.keyframes.size()));
    for (const auto& kf : spec.keyframes) {
        builder.add_f64(kf.time);
        builder.add_u32(kf.value.r);
        builder.add_u32(kf.value.g);
        builder.add_u32(kf.value.b);
        builder.add_u32(kf.value.a);
        builder.add_u32(static_cast<std::uint32_t>(kf.easing));
        builder.add_u64(static_cast<std::uint64_t>(kf.bezier.cx1 * 1000000.0));
        builder.add_u64(static_cast<std::uint64_t>(kf.bezier.cy1 * 1000000.0));
        builder.add_u64(static_cast<std::uint64_t>(kf.bezier.cx2 * 1000000.0));
        builder.add_u64(static_cast<std::uint64_t>(kf.bezier.cy2 * 1000000.0));
        builder.add_f64(kf.speed_in);
        builder.add_f64(kf.influence_in);
        builder.add_f64(kf.speed_out);
        builder.add_f64(kf.influence_out);
    }
}

CompositionSummary make_summary(const CompositionSpec& composition) {
    CompositionSummary summary;
    summary.id = composition.id;
    summary.name = composition.name;
    summary.width = composition.width;
    summary.height = composition.height;
    summary.duration = composition.duration;
    summary.frame_rate = composition.frame_rate;
    summary.background = composition.background;
    summary.layer_count = composition.layers.size();
    summary.solid_layer_count = count_layers_with_type(composition, "solid");
    summary.shape_layer_count = count_layers_with_type(composition, "shape");
    summary.mask_layer_count = count_layers_with_type(composition, "mask");
    summary.image_layer_count = count_layers_with_type(composition, "image");
    summary.text_layer_count = count_layers_with_type(composition, "text");
    summary.precomp_layer_count = count_layers_with_type(composition, "precomp");
    summary.track_matte_layer_count = count_layers_with_track_matte(composition);
    return summary;
}

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
            hash_animated_scalar(builder, layer.opacity_property);
            hash_animated_scalar(builder, layer.mask_feather);
            hash_animated_scalar(builder, layer.time_remap_property);
            hash_animated_scalar(builder, layer.font_size);
            hash_animated_scalar(builder, layer.stroke_width_property);
            hash_animated_scalar(builder, layer.repeater_count);
            hash_animated_scalar(builder, layer.repeater_stagger_delay);
            hash_animated_scalar(builder, layer.repeater_offset_position_x);
            hash_animated_scalar(builder, layer.repeater_offset_position_y);
            hash_animated_scalar(builder, layer.repeater_offset_rotation);
            hash_animated_scalar(builder, layer.repeater_offset_scale_x);
            hash_animated_scalar(builder, layer.repeater_offset_scale_y);
            hash_animated_scalar(builder, layer.repeater_start_opacity);
            hash_animated_scalar(builder, layer.repeater_end_opacity);
            hash_animated_scalar(builder, layer.light_intensity);
            hash_animated_scalar(builder, layer.attenuation_near);
            hash_animated_scalar(builder, layer.attenuation_far);
            hash_animated_scalar(builder, layer.cone_angle);
            hash_animated_scalar(builder, layer.cone_feather);
            hash_animated_scalar(builder, layer.shadow_darkness);
            hash_animated_scalar(builder, layer.shadow_radius);
            hash_animated_scalar(builder, layer.camera_zoom);
            hash_animated_vector3(builder, layer.camera_poi);
            hash_animated_scalar(builder, layer.camera_shake_amplitude_pos);
            hash_animated_scalar(builder, layer.camera_shake_amplitude_rot);
            hash_animated_scalar(builder, layer.camera_shake_frequency);
            hash_animated_scalar(builder, layer.camera_shake_roughness);
            hash_animated_color(builder, layer.fill_color);
            hash_animated_color(builder, layer.stroke_color);
            hash_animated_color(builder, layer.light_color);
            builder.add_u64(static_cast<std::uint64_t>(layer.effects.size()));
            for (const auto& effect : layer.effects) hash_effect(builder, effect);
            builder.add_u64(static_cast<std::uint64_t>(layer.text_animators.size()));
            for (const auto& animator : layer.text_animators) {
                builder.add_string(animator.name);
                builder.add_string(animator.selector.type);
                builder.add_f64(animator.selector.start);
                builder.add_f64(animator.selector.end);
                builder.add_bool(animator.properties.opacity_value.has_value());
                if (animator.properties.opacity_value) builder.add_f64(*animator.properties.opacity_value);
                builder.add_u64(static_cast<std::uint64_t>(animator.properties.opacity_keyframes.size()));
                builder.add_bool(animator.properties.fill_color_value.has_value());
                if (animator.properties.fill_color_value) {
                    builder.add_u32(animator.properties.fill_color_value->r);
                    builder.add_u32(animator.properties.fill_color_value->g);
                    builder.add_u32(animator.properties.fill_color_value->b);
                    builder.add_u32(animator.properties.fill_color_value->a);
                }
                builder.add_u64(static_cast<std::uint64_t>(animator.properties.fill_color_keyframes.size()));
            }
            builder.add_u64(static_cast<std::uint64_t>(layer.text_highlights.size()));
            for (const auto& highlight : layer.text_highlights) {
                builder.add_u64(highlight.start_glyph);
                builder.add_u64(highlight.end_glyph);
                builder.add_u32(highlight.color.r);
                builder.add_u32(highlight.color.g);
                builder.add_u32(highlight.color.b);
                builder.add_u32(highlight.color.a);
            }
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

} // namespace tachyon
