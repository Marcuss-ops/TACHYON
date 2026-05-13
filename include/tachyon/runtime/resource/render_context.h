#pragma once

#ifdef TACHYON_ENABLE_MEDIA
#include "tachyon/media/management/media_manager.h"
#endif

#include "tachyon/runtime/execution/planning/quality_policy.h"
#include "tachyon/runtime/resource/surface_pool.h"

#ifdef _WIN32
#include <OpenImageDenoise/oidn.hpp>
#endif

#include "tachyon/runtime/core/diagnostics/diagnostics.h"
#include <memory>
#include <atomic>
#include <functional>
#include <vector>

#include "tachyon/renderer2d/core/render_types.h"
#include "tachyon/renderer2d/color/color_management_system.h"

namespace tachyon {

namespace renderer2d {
class EffectHost;
class PrecompCache;
class ComputeBackend;
}

namespace text {
class FontRegistry;
struct SubtitleEntry;
}

namespace presets {
class TextRegistry;
}

class TransitionRegistry;

namespace profiling { class RenderProfiler; }

namespace media { 
#ifdef TACHYON_ENABLE_MEDIA
class MediaManager; 
class MediaPrefetcher; 
class PlaybackScheduler; 
class AssetResolver;
#endif
}
namespace profiling { class RenderProfiler; }

/**
 * @brief Unified Render Context for both Runtime and Renderer2D.
 */
struct RenderContext {
    // 1. Core Resources
    std::shared_ptr<SurfacePool> surface_pool;
    std::shared_ptr<renderer2d::PrecompCache> precomp_cache;
    std::shared_ptr<renderer2d::PrecompCache> text_surface_cache;
    
    // 2. Engine Components
    std::shared_ptr<renderer2d::EffectHost> effects;
    const TransitionRegistry* transition_registry{nullptr};
    const presets::TextRegistry* text_registry{nullptr};
    const text::FontRegistry* font_registry{nullptr};

    // 3. Media & Audio
#ifdef TACHYON_ENABLE_MEDIA
    std::shared_ptr<media::MediaManager> media;
    std::shared_ptr<media::AssetResolver> asset_resolver;
    media::MediaPrefetcher* prefetcher{nullptr};
    media::PlaybackScheduler* scheduler{nullptr};
#endif
    const std::vector<text::SubtitleEntry>* subtitle_entries{nullptr};

    // 4. State & Configuration
    QualityPolicy policy;
    renderer2d::ColorManagementSystem cms;
    renderer2d::WorkingColorSpace working_color_space;
    renderer2d::AccumulationBuffers accumulation_buffer;
    
    int width{0};
    int height{0};
    std::size_t pixel_concurrency{1};
    // Optional tile-level parallel executor. When set, composition_renderer uses it
    // instead of the raw OpenMP pragma for tile dispatch.
    // Signature: executor(tile_count, fn) where fn(tile_index) renders one tile.
    std::function<void(std::size_t, const std::function<void(std::size_t)>&)> tile_executor;

    // 5. External Services
#ifdef _WIN32
    oidn::DeviceRef oidn_device;
    oidn::FilterRef oidn_filter;
#endif
    FrameDiagnostics* diagnostics{nullptr};
    profiling::RenderProfiler* profiler{nullptr};
    std::atomic<bool>* cancel_flag{nullptr};

    // 6. Backend
    std::shared_ptr<renderer2d::ComputeBackend> compute_backend;

    explicit RenderContext(
        std::shared_ptr<renderer2d::PrecompCache> precomp_cache = nullptr
#ifdef TACHYON_ENABLE_MEDIA
        , std::shared_ptr<media::MediaManager> media_mgr = nullptr
#endif
    );
};

using RenderContext2D = RenderContext;

} // namespace tachyon
