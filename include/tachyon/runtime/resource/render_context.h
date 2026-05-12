#pragma once

#ifdef TACHYON_ENABLE_MEDIA
#include "tachyon/media/management/media_manager.h"
#endif
#include "tachyon/renderer2d/resource/render_context.h"

#include "tachyon/runtime/execution/planning/quality_policy.h"
#include "tachyon/runtime/resource/runtime_surface_pool.h"

#ifdef _WIN32
#include <OpenImageDenoise/oidn.hpp>
#endif

#include "tachyon/runtime/core/diagnostics/diagnostics.h"
#include <memory>
#include <atomic>

namespace tachyon {

namespace media { 
#ifdef TACHYON_ENABLE_MEDIA
class MediaManager; 
class MediaPrefetcher; 
class PlaybackScheduler; 
#endif
}
namespace profiling { class RenderProfiler; }

struct RenderContext {
#ifdef TACHYON_ENABLE_MEDIA
    std::shared_ptr<media::MediaManager> media;
    media::MediaPrefetcher* prefetcher{nullptr};
    media::PlaybackScheduler* scheduler{nullptr};
#endif
    renderer2d::RenderContext2D renderer2d;
    QualityPolicy policy;

#ifdef _WIN32
    oidn::DeviceRef oidn_device;
    oidn::FilterRef oidn_filter;
#endif
    runtime::RuntimeSurfacePool* surface_pool{nullptr};
    FrameDiagnostics* diagnostic_tracker{nullptr};
    profiling::RenderProfiler* profiler{nullptr};
    std::atomic<bool>* cancel_flag{nullptr};

    explicit RenderContext(
        std::shared_ptr<renderer2d::PrecompCache> precomp_cache = nullptr
#ifdef TACHYON_ENABLE_MEDIA
        , std::shared_ptr<media::MediaManager> media_mgr = nullptr
#endif
    );

};

} // namespace tachyon
