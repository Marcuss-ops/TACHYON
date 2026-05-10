#include "tachyon/runtime/cache/disk_frame_cache_adapter.h"
#include "tachyon/runtime/cache/disk_cache.h"
#include <vector>
#include <cstdint>
#include <cstring>

namespace tachyon::runtime {

struct FrameHeader {
    uint32_t magic = 0x54414348; // 'TACH'
    uint16_t version = 1;
    uint32_t width;
    uint32_t height;
    renderer2d::ColorProfile profile;
    uint64_t pixels_size;
    uint64_t depth_size;
};

DiskFrameCacheAdapter::DiskFrameCacheAdapter(DiskCacheStore& disk_store)
    : m_disk_store(disk_store) {}

std::optional<std::shared_ptr<renderer2d::Framebuffer>> DiskFrameCacheAdapter::load(
    const CompiledScene& scene,
    const RenderPlan& plan,
    const FrameRenderTask& task
) {
    auto disk_key = to_disk_key(scene, plan, task);
    auto data = m_disk_store.load(disk_key);
    if (!data.has_value() || data->size() < sizeof(FrameHeader)) {
        return std::nullopt;
    }

    FrameHeader header;
    std::memcpy(&header, data->data(), sizeof(FrameHeader));

    if (header.magic != 0x54414348 || header.version != 1) {
        return std::nullopt;
    }

    auto frame = std::make_shared<renderer2d::Framebuffer>(header.width, header.height);
    frame->set_profile(header.profile);

    const uint8_t* payload = data->data() + sizeof(FrameHeader);
    
    if (header.pixels_size > 0) {
        frame->mutable_pixels().resize(header.pixels_size / sizeof(float));
        std::memcpy(frame->mutable_pixels().data(), payload, header.pixels_size);
        payload += header.pixels_size;
    }

    return frame;
}

void DiskFrameCacheAdapter::store(
    const CompiledScene& scene,
    const RenderPlan& plan,
    const FrameRenderTask& task,
    const renderer2d::Framebuffer& frame
) {
    auto disk_key = to_disk_key(scene, plan, task);
    
    FrameHeader header;
    header.width = frame.width();
    header.height = frame.height();
    header.profile = frame.profile();
    header.pixels_size = frame.pixels().size() * sizeof(float);
    header.depth_size = 0; 

    std::vector<uint8_t> frame_data(sizeof(FrameHeader) + header.pixels_size);
    std::memcpy(frame_data.data(), &header, sizeof(FrameHeader));
    std::memcpy(frame_data.data() + sizeof(FrameHeader), frame.pixels().data(), header.pixels_size);

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
