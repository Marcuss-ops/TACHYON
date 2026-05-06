#include "tachyon/runtime/cache/disk_frame_cache_adapter.h"
#include "tachyon/runtime/cache/disk_cache.h"
#include <vector>
#include <cstdint>

namespace tachyon::runtime {

DiskFrameCacheAdapter::DiskFrameCacheAdapter(DiskCacheStore& disk_store)
    : m_disk_store(disk_store) {}

std::optional<std::shared_ptr<renderer2d::Framebuffer>> DiskFrameCacheAdapter::load(
    const CompiledScene& scene,
    const RenderPlan& plan,
    const FrameRenderTask& task
) {
    auto disk_key = to_disk_key(scene, plan, task);
    auto data = m_disk_store.load(disk_key);
    if (!data.has_value()) {
        return std::nullopt;
    }
    // TODO: Implement framebuffer deserialization from vector<uint8_t>
    return std::nullopt;
}

void DiskFrameCacheAdapter::store(
    const CompiledScene& scene,
    const RenderPlan& plan,
    const FrameRenderTask& task,
    const renderer2d::Framebuffer& frame
) {
    auto disk_key = to_disk_key(scene, plan, task);
    // TODO: Implement framebuffer serialization to vector<uint8_t>
    std::vector<uint8_t> frame_data;
    m_disk_store.store(disk_key, frame_data);
}

disk_cache::CacheKey DiskFrameCacheAdapter::to_disk_key(
    const CompiledScene& scene,
    const RenderPlan& plan,
    const FrameRenderTask& task
) const {
    FrameCacheKeyFactory factory(scene, plan);
    auto runtime_key = factory.frame_key(task);

    disk_cache::CacheKey key;
    key.identity = runtime_key;
    key.node_path = plan.composition_target;
    const double fps = plan.composition.fps > 0 ? plan.composition.fps : 60.0;
    key.frame_time = static_cast<double>(task.frame_number) / fps;
    key.quality_tier = 1; // Map from plan.quality_tier (string to int)
    key.aov_type = "beauty";
    key.time_remap_mode = plan.time_remap_curve.has_value()
        ? plan.time_remap_curve->mode
        : spec::TimeRemapMode::Blend;
    key.frame_blend_mode = plan.frame_blend_mode;
    key.frame_blend_strength = 1.0f;
    key.shutter_angle_deg = static_cast<float>(plan.motion_blur_shutter_angle);
    key.motion_blur_samples = static_cast<int>(plan.motion_blur_samples);
    key.motion_blur_seed = 0;
    key.use_proxy = plan.proxy_enabled;
    key.proxy_variant_id = "";

    return key;
}

} // namespace tachyon::runtime
