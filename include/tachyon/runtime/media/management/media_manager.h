#pragma once

#include "tachyon/runtime/media/management/image_manager.h"
#include "tachyon/media/decoding/video_decoder.h"
#ifdef TACHYON_ENABLE_AUDIO
#include "tachyon/audio/audio_mixer.h"
#endif
#include "tachyon/renderer2d/core/framebuffer.h"
#include "tachyon/renderer2d/core/render_types.h"

#include "tachyon/runtime/core/diagnostics/diagnostics.h"

#include "tachyon/runtime/media/management/media_asset.h"
#include "tachyon/runtime/media/management/proxy_manifest.h"
#include "tachyon/runtime/media/management/proxy_worker.h"
#include "tachyon/runtime/media/management/resource_cache.h"
#include "tachyon/runtime/media/resolution/path_resolver.h"
#include "tachyon/core/media/resolved_asset.h"
#include <filesystem>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <functional>

#include "tachyon/core/media/media_provider.h"

namespace tachyon::media {

enum class MediaFallbackPolicy {
    ReturnError,      // Fail if source missing
    ReturnOffline,    // Show "Offline" graphic
    UseProxy,         // Use proxy if original missing (default)
    UseOriginal       // Use original even if proxy requested
};

enum class ResolutionPurpose {
    Playback,
    Export,
    Analysis
};

class MediaManager : public IMediaProvider {
public:
    MediaManager();
    ~MediaManager();

    void set_fallback_policy(MediaFallbackPolicy policy) { m_fallback_policy = policy; }
    MediaFallbackPolicy fallback_policy() const { return m_fallback_policy; }

    const renderer2d::SurfaceRGBA* get_video_frame(
        const std::filesystem::path& path,
        double time,
        ::tachyon::DiagnosticBag* diagnostics = nullptr) override;

    void set_memory_budget(std::size_t bytes);
    std::size_t current_memory_usage() const;

    const renderer2d::SurfaceRGBA* get_image(
        const std::filesystem::path& path, 
        ::tachyon::AlphaMode alpha_mode = ::tachyon::AlphaMode::Straight,
        ::tachyon::DiagnosticBag* diagnostics = nullptr) override;

    double get_duration(const std::filesystem::path& path) override;

    std::filesystem::path resolve_media_path(const std::filesystem::path& path) const override {
        return resolve_media_path_detailed(path, ResolutionPurpose::Playback);
    }


    /**
     * @brief Registers an asset in the manager.
     */
    void register_asset(std::shared_ptr<MediaAsset> asset);

    /**
     * @brief Gets the path for an asset by its ID.
     * @return The original path of the asset, or empty path if not found.
     */
    std::filesystem::path get_asset_path(const std::string& asset_id) const;

    /**
     * @brief Resolves the best available path for an asset.
     */
    std::filesystem::path resolve_media_path_detailed(
        const std::filesystem::path& path,
        ResolutionPurpose purpose = ResolutionPurpose::Playback,
        uint32_t target_width = 0) const;

    /**
     * @brief Load image from resolved asset.
     */
    const renderer2d::SurfaceRGBA* get_image(
        const ResolvedAsset& asset,
        ::tachyon::AlphaMode alpha_mode = ::tachyon::AlphaMode::Straight,
        DiagnosticBag* diagnostics = nullptr);

    /**
     * @brief Get video frame from resolved asset.
     */
    const renderer2d::SurfaceRGBA* get_video_frame(
        const ResolvedAsset& asset,
        double time,
        DiagnosticBag* diagnostics = nullptr);

    /**
     * @brief Get HDR image from resolved asset.
     */

    // Mesh loading removed to streamline core.


    /**
     * Acquires a VideoDecoder for the given path from a pool.
     */
    using PooledVideoDecoder = std::unique_ptr<VideoDecoder, std::function<void(VideoDecoder*)>>;
    PooledVideoDecoder acquire_video_decoder(const std::filesystem::path& path);
    void release_video_decoder(const std::filesystem::path& path, VideoDecoder* decoder);



    DiagnosticBag consume_diagnostics();
    void store_video_frame(const std::filesystem::path& path, double time, std::unique_ptr<renderer2d::SurfaceRGBA> frame);
    void clear_cache();

#ifdef TACHYON_ENABLE_AUDIO
    std::shared_ptr<audio::AudioMixer> audio_mixer() { return m_audio_mixer; }
    void set_audio_mixer(std::shared_ptr<audio::AudioMixer> mixer) { m_audio_mixer = mixer; }
#endif

    ProxyManifest& proxy_manifest() { return m_proxy_manifest; }
    ProxyWorker& proxy_worker() { return *m_proxy_worker; }

    void set_use_proxies(bool use) { m_use_proxies = use; }
    bool use_proxies() const { return m_use_proxies; }

    void set_placeholder_size(uint32_t width, uint32_t height) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_placeholder_width = width;
        m_placeholder_height = height;
    }

    /**
     * @brief Queue proxy generation for a list of files.
     */
    void generate_proxies(const std::vector<std::string>& originals, const ProxyPolicy& policy) {
        m_proxy_worker->generate_proxies(originals, policy);
    }

    ImageManager& image_manager() { return m_image_manager; }
    const ImageManager& image_manager() const { return m_image_manager; }

private:
    struct VideoPool {
        std::vector<std::unique_ptr<VideoDecoder>> available;
        std::mutex mutex;
    };

    ImageManager m_image_manager;
    std::map<std::string, std::shared_ptr<VideoPool>> m_video_pools;
    std::unique_ptr<ResourceCache<std::string, renderer2d::SurfaceRGBA>> m_frame_cache;
    std::unordered_map<std::string, std::shared_ptr<MediaAsset>> m_assets;

    
    MediaFallbackPolicy m_fallback_policy{MediaFallbackPolicy::UseProxy};
    std::shared_ptr<renderer2d::SurfaceRGBA> m_offline_placeholder;
    uint32_t m_placeholder_width{1920};
    uint32_t m_placeholder_height{1080};

    mutable std::mutex m_mutex;
    bool m_use_proxies{true};
#ifdef TACHYON_ENABLE_AUDIO
    std::shared_ptr<audio::AudioMixer> m_audio_mixer;
#endif

    ProxyManifest m_proxy_manifest;
    std::unique_ptr<ProxyWorker> m_proxy_worker;
    PathResolver m_path_resolver;
};

} // namespace tachyon::media
