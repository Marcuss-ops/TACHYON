#include "tachyon/core/spec/compilation/scene_hash_builder.h"
#include "tachyon/runtime/cache/cache_key_builder.h"
#include <string>
#include <string_view>

namespace tachyon {
namespace {

void add_string(CacheKeyBuilder& builder, const std::string& value) {
    builder.add_string(std::string_view{value});
}

} // namespace

std::uint64_t hash_scene_spec(const SceneSpec& scene, const DeterminismContract& contract) {
    CacheKeyBuilder builder;
    builder.add_string(scene.schema_version.to_string());
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
            builder.add_u32(static_cast<std::uint32_t>(composition.background->type));
            add_string(builder, composition.background->value);
            if (composition.background->parsed_color.has_value()) {
                const auto& c = *composition.background->parsed_color;
                builder.add_u32(c.r);
                builder.add_u32(c.g);
                builder.add_u32(c.b);
                builder.add_u32(c.a);
            }
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

} // namespace tachyon
