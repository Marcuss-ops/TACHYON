#include "tachyon/runtime/execution/planning/render_plan.h"
#include "tachyon/runtime/cache/cache_key_builder.h"

#include <algorithm>
#include <string_view>

namespace tachyon {
namespace {

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

} // namespace

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
            for (const auto& animator : layer.text_animators) builder.add_string(animator);
            builder.add_u64(static_cast<std::uint64_t>(layer.text_highlights.size()));
            for (const auto& highlight : layer.text_highlights) builder.add_string(highlight);
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

FrameCacheKey build_frame_cache_key(const RenderPlan& plan, std::int64_t frame_number) {
    CacheKeyBuilder builder;
    builder.add_string(plan.job_id);
    builder.add_string(plan.scene_ref);
    builder.add_string(plan.composition_target);
    builder.add_string(plan.quality_tier);
    builder.add_string(plan.compositing_alpha_mode);
    builder.add_string(plan.working_space);
    builder.add_string(plan.motion_blur_curve);
    builder.add_string(plan.seed_policy_mode);
    builder.add_string(plan.compatibility_mode);
    builder.add_u64(static_cast<std::uint64_t>(plan.composition.width));
    builder.add_u64(static_cast<std::uint64_t>(plan.composition.height));
    builder.add_u64(static_cast<std::uint64_t>(plan.frame_range.start));
    builder.add_u64(static_cast<std::uint64_t>(plan.frame_range.end));
    builder.add_u64(static_cast<std::uint64_t>(frame_number));
    builder.add_u64(static_cast<std::uint64_t>(plan.motion_blur_enabled ? 1U : 0U));
    builder.add_u64(static_cast<std::uint64_t>(static_cast<std::uint32_t>(plan.motion_blur_mode)));
    builder.add_u64(static_cast<std::uint64_t>(plan.motion_blur_samples));
    builder.add_u64(static_cast<std::uint64_t>(plan.motion_blur_shutter_angle * 1000.0));
    builder.add_u64(static_cast<std::uint64_t>(plan.motion_blur_shutter_phase * 1000.0));
    builder.add_u64(static_cast<std::uint64_t>(plan.composition.layer_count));
    builder.add_u64(static_cast<std::uint64_t>(plan.composition.text_layer_count));
    builder.add_u64(static_cast<std::uint64_t>(plan.composition.precomp_layer_count));

    // New Canonical Fields
    builder.add_u64(plan.scene_hash);
    builder.add_u32(static_cast<std::uint32_t>(plan.contract_version));
    builder.add_bool(plan.proxy_enabled);
    builder.add_string(plan.ocio.config_path);
    builder.add_string(plan.ocio.display);
    builder.add_string(plan.ocio.view);
    builder.add_string(plan.ocio.look);
    builder.add_bool(plan.dof.enabled);
    builder.add_f64(plan.dof.aperture);
    builder.add_f64(plan.dof.focus_distance);
    builder.add_f64(plan.dof.focal_length);

    // Time Remap & Frame Blend
    if (plan.time_remap_curve.has_value()) {
        builder.add_bool(true);
        builder.add_bool(plan.time_remap_curve->enabled);
        builder.add_u32(static_cast<std::uint32_t>(plan.time_remap_curve->mode));
        for (const auto& kf : plan.time_remap_curve->keyframes) {
            builder.add_f32(kf.first);
            builder.add_f32(kf.second);
        }
    } else {
        builder.add_bool(false);
    }
    builder.add_u32(static_cast<std::uint32_t>(plan.frame_blend_mode));

    // Variables (sorted for stability)
    std::vector<std::string> var_keys;
    for (const auto& [k, v] : plan.variables) var_keys.push_back(k);
    std::sort(var_keys.begin(), var_keys.end());
    for (const auto& k : var_keys) {
        builder.add_string(k);
        builder.add_f64(plan.variables.at(k));
    }

    std::vector<std::string> s_var_keys;
    for (const auto& [k, v] : plan.string_variables) s_var_keys.push_back(k);
    std::sort(s_var_keys.begin(), s_var_keys.end());
    for (const auto& k : s_var_keys) {
        builder.add_string(k);
        builder.add_string(plan.string_variables.at(k));
    }

    FrameCacheKey key;
    key.hash = builder.finish();
    key.value =
        plan.job_id + ":" +
        plan.scene_ref + ":" +
        plan.composition_target + ":" +
        plan.quality_tier + ":" +
        plan.compositing_alpha_mode + ":" +
        plan.working_space + ":" +
        plan.motion_blur_curve + ":" +
        plan.seed_policy_mode + ":" +
        plan.compatibility_mode + ":" +
        std::to_string(plan.composition.width) + "x" +
        std::to_string(plan.composition.height) + ":" +
        std::to_string(plan.frame_range.start) + "-" +
        std::to_string(plan.frame_range.end) + ":f" +
        std::to_string(frame_number) + ":mb" +
        std::to_string(plan.motion_blur_enabled ? 1 : 0) + ":" +
        std::to_string(plan.motion_blur_samples) + ":" +
        std::to_string(static_cast<std::int64_t>(plan.motion_blur_shutter_angle * 1000.0)) + ":" +
        std::to_string(static_cast<std::uint32_t>(plan.frame_blend_mode));
    return key;
}

bool frame_cache_entry_matches(const FrameCacheEntry& entry, const FrameCacheKey& key) {
    return entry.key.hash == key.hash && entry.key.value == key.value;
}

} // namespace tachyon
