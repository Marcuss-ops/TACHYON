#include "tachyon/runtime/execution/frame_cache_key_factory.h"
#include "tachyon/runtime/execution/planning/render_plan.h"
#include "tachyon/runtime/core/graph/compiled_scene.h"
#include "tachyon/runtime/core/graph/compiled_node.h"
#include "tachyon/runtime/execution/frame_render_task.h"
#include "tachyon/runtime/cache/cache_key_builder.h"

#include <algorithm>
#include <optional>
#include <string>
#include <vector>

namespace tachyon::runtime {

FrameCacheKeyFactory::FrameCacheKeyFactory(const CompiledScene& scene, const RenderPlan& plan)
    : m_scene(scene), m_plan(plan), m_cached_composition_key(std::nullopt) {}

std::uint64_t FrameCacheKeyFactory::composition_key() const {
    if (m_cached_composition_key.has_value()) {
        return *m_cached_composition_key;
    }

    CacheKeyBuilder builder;
    builder.enable_manifest(m_manifest_enabled);
    // Scene-level fields
    builder.add_u64(m_scene.scene_hash);
    builder.add_u32(m_scene.header.layout_version);
    builder.add_u32(m_scene.header.compiler_version);

    // Plan-level fields (shared across all frames)
    builder.add_string(m_plan.quality_tier);
    builder.add_string(m_plan.compositing_alpha_mode);
    builder.add_string(m_plan.working_space);
    builder.add_string(m_plan.motion_blur_curve);
    builder.add_string(m_plan.seed_policy_mode);
    builder.add_string(m_plan.compatibility_mode);
    builder.add_u64(static_cast<std::uint64_t>(m_plan.composition.width));
    builder.add_u64(static_cast<std::uint64_t>(m_plan.composition.height));
    builder.add_u64(static_cast<std::uint64_t>(m_plan.frame_range.start));
    builder.add_u64(static_cast<std::uint64_t>(m_plan.frame_range.end));
    builder.add_u64(static_cast<std::uint64_t>(m_plan.motion_blur_enabled ? 1U : 0U));
    builder.add_u64(static_cast<std::uint64_t>(static_cast<std::uint32_t>(m_plan.motion_blur_mode)));
    builder.add_u64(static_cast<std::uint64_t>(m_plan.motion_blur_samples));
    builder.add_u64(static_cast<std::uint64_t>(m_plan.motion_blur_shutter_angle * 1000.0));
    builder.add_u64(static_cast<std::uint64_t>(m_plan.motion_blur_shutter_phase * 1000.0));
    builder.add_u64(static_cast<std::uint64_t>(m_plan.composition.layer_count));
    builder.add_u64(static_cast<std::uint64_t>(m_plan.composition.text_layer_count));
    builder.add_u64(static_cast<std::uint64_t>(m_plan.composition.precomp_layer_count));
    builder.add_u64(m_plan.scene_hash);
    builder.add_u64(m_plan.font_content_hash);
    builder.add_u32(static_cast<std::uint32_t>(m_plan.contract_version));
    builder.add_bool(m_plan.proxy_enabled);
    builder.add_string(m_plan.ocio.config_path);
    builder.add_string(m_plan.ocio.display);
    builder.add_string(m_plan.ocio.view);
    builder.add_string(m_plan.ocio.look);
    builder.add_bool(m_plan.dof.enabled);
    builder.add_f64(m_plan.dof.aperture);
    builder.add_f64(m_plan.dof.focus_distance);
    builder.add_f64(m_plan.dof.focal_length);

    // Time Remap
    if (m_plan.time_remap_curve.has_value()) {
        builder.add_bool(true);
        builder.add_bool(m_plan.time_remap_curve->enabled);
        builder.add_u32(static_cast<std::uint32_t>(m_plan.time_remap_curve->mode));
        for (const auto& kf : m_plan.time_remap_curve->keyframes) {
            builder.add_f32(kf.first);
            builder.add_f32(kf.second);
        }
    } else {
        builder.add_bool(false);
    }
    builder.add_u32(static_cast<std::uint32_t>(m_plan.frame_blend_mode));

    // Variables (sorted for stability)
    std::vector<std::string> var_keys;
    for (const auto& [k, v] : m_plan.variables) var_keys.push_back(k);
    std::sort(var_keys.begin(), var_keys.end());
    for (const auto& k : var_keys) {
        builder.add_string(k);
        builder.add_f64(m_plan.variables.at(k));
    }

    std::vector<std::string> s_var_keys;
    for (const auto& [k, v] : m_plan.string_variables) s_var_keys.push_back(k);
    std::sort(s_var_keys.begin(), s_var_keys.end());
    for (const auto& k : s_var_keys) {
        builder.add_string(k);
        builder.add_string(m_plan.string_variables.at(k));
    }

    if (m_manifest_enabled) {
        m_manifest = builder.manifest();
    }
    m_cached_composition_key = builder.finish();
    return *m_cached_composition_key;
}

std::uint64_t FrameCacheKeyFactory::frame_key(const FrameRenderTask& task) const {
    CacheKeyBuilder builder;
    builder.add_u64(composition_key());
    builder.add_u64(static_cast<std::uint64_t>(task.frame_number));
    return builder.finish();
}

std::uint64_t FrameCacheKeyFactory::time_sample_key(
    const FrameRenderTask& task,
    double render_time,
    std::optional<std::uint64_t> sample_index
) const {
    CacheKeyBuilder builder;
    builder.add_u64(composition_key());
    builder.add_u64(static_cast<std::uint64_t>(task.frame_number));
    builder.add_f64(render_time);
    if (sample_index.has_value()) {
        builder.add_u64(*sample_index);
    }
    return builder.finish();
}

std::uint64_t FrameCacheKeyFactory::node_key(std::uint64_t frame_key, const CompiledNode& node) const {
    CacheKeyBuilder builder;
    builder.add_u64(frame_key);
    builder.add_u32(node.node_id);
    builder.add_u32(node.topo_index);
    builder.add_u32(node.version);
    return builder.finish();
}

} // namespace tachyon::runtime
