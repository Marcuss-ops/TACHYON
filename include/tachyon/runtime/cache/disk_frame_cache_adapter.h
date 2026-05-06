#pragma once

#include "tachyon/runtime/execution/frame_cache_key_factory.h"
#include "tachyon/runtime/cache/disk_cache.h"
#include "tachyon/runtime/execution/planning/render_plan.h"
#include "tachyon/runtime/core/graph/compiled_scene.h"
#include "tachyon/runtime/execution/frame_render_task.h"
#include "tachyon/renderer2d/core/framebuffer.h"

#include <optional>
#include <vector>

namespace tachyon::runtime {

class DiskFrameCacheAdapter {
public:
    explicit DiskFrameCacheAdapter(DiskCacheStore& disk_store);

    std::optional<std::shared_ptr<renderer2d::Framebuffer>> load(
        const CompiledScene& scene,
        const RenderPlan& plan,
        const FrameRenderTask& task
    );

    void store(
        const CompiledScene& scene,
        const RenderPlan& plan,
        const FrameRenderTask& task,
        const renderer2d::Framebuffer& frame
    );

private:
    DiskCacheStore& m_disk_store;

    disk_cache::CacheKey to_disk_key(
        const CompiledScene& scene,
        const RenderPlan& plan,
        const FrameRenderTask& task
    ) const;
};

} // namespace tachyon::runtime
