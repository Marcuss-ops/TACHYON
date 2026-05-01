#pragma once

#include "tachyon/media/management/media_manager.h"
#include "tachyon/renderer2d/resource/render_context.h"
#include "tachyon/renderer3d/core/ray_tracer.h"
#include "tachyon/runtime/execution/planning/quality_policy.h"
#include "tachyon/runtime/resource/runtime_surface_pool.h"

#ifdef _WIN32
#include <OpenImageDenoise/oidn.hpp>
#endif

#include "tachyon/runtime/core/diagnostics/diagnostics.h"
#include <memory>

namespace tachyon {

namespace media { class MediaPrefetcher; class PlaybackScheduler; }

struct RenderContext {
    std::shared_ptr<media::MediaManager> media;
    media::MediaPrefetcher* prefetcher{nullptr};
    media::PlaybackScheduler* scheduler{nullptr};
    renderer2d::RenderContext2D renderer2d;
    QualityPolicy policy;
    std::shared_ptr<renderer3d::RayTracer> ray_tracer;
#ifdef _WIN32
    oidn::DeviceRef oidn_device;
    oidn::FilterRef oidn_filter;
#endif
    runtime::RuntimeSurfacePool* surface_pool{nullptr};
    FrameDiagnostics* diagnostic_tracker{nullptr};

    explicit RenderContext(std::shared_ptr<renderer2d::PrecompCache> precomp_cache = nullptr);
};

} // namespace tachyon
