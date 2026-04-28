#include "tachyon/runtime/execution/planning/render_plan.h"
#include "tachyon/runtime/cache/cache_key_builder.h"
#include "src/runtime/execution/planning/hash/hash_utils.h"

#include <algorithm>
#include <string_view>

namespace tachyon {

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
