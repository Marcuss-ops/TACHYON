#pragma once

#include "tachyon/media/management/image_manager.h"
#include "tachyon/media/loading/mesh_asset.h"
#include "tachyon/media/decoding/video_decoder.h"
#include "tachyon/audio/audio_mixer.h"
#include "tachyon/renderer2d/core/framebuffer.h"
#include "tachyon/runtime/core/diagnostics/diagnostics.h"

#include "tachyon/media/management/media_asset.h"
#include "tachyon/media/management/proxy_manifest.h"
#include "tachyon/media/management/proxy_worker.h"
#include "tachyon/media/management/resource_cache.h"
#include "tachyon/media/path_resolver.h"
#include "tachyon/media/resolved_asset.h"
#include <filesystem>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <functional>

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

class MediaManager {
public:
    MediaManager();
    ~MediaManager();

    void set_fallback_policy(MediaFallbackPolicy policy) { m_fallback_policy = policy; }
    MediaFallbackPolicy fallback_policy() const { return m_fallback_policy; }

    const renderer2d::SurfaceRGBA* get_video_frame(
        const std::filesystem::path& path,
        double time,
        DiagnosticBag* diagnostics = nullptr);

    void set_memory_budget(std::size_t bytes);
    std::size_t current_memory_usage() const;

    const renderer2d::SurfaceRGBA* get_image(
        const std::filesystem::path& path, 
        AlphaMode alpha_mode = AlphaMode::Straight,
        DiagnosticBag* diagnostics = nullptr);

    const HDRTextureData* get_hdr_image(
        const std::filesystem::path& path,
        DiagnosticBag* diagnostics = nullptr);

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
    std::filesystem::path resolve_media_path(
        const std::filesystem::path& path,
        ResolutionPurpose purpose = ResolutionPurpose::Playback) const;

    /**
     * @brief Load image from resolved asset.
     */
    const renderer2d::SurfaceRGBA* get_image(
        const ResolvedAsset& asset,
        AlphaMode alpha_mode = AlphaMode::Straight,
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
    const HDRTextureData* get_hdr_image(
        const ResolvedAsset& asset,
        DiagnosticBag* diagnostics = nullptr);

    /**
     * @brief Get mesh from resolved asset.
     */
    std::shared_ptr<const MeshAsset> get_mesh(
        const ResolvedAsset& asset,
        DiagnosticBag* diagnostics = nullptr);

    /**
     * Acquires a VideoDecoder for the given path from a pool.
     */
    using PooledVideoDecoder = std::unique_ptr<VideoDecoder, std::function<void(VideoDecoder*)>>;
    PooledVideoDecoder acquire_video_decoder(const std::filesystem::path& path);
    void release_video_decoder(const std::filesystem::path& path, VideoDecoder* decoder);

    std::shared_ptr<const MeshAsset> get_mesh(
        const std::filesystem::path& path,
        DiagnosticBag* diagnostics = nullptr);

    DiagnosticBag consume_diagnostics();
    void store_video_frame(const std::string& path, double time, std::unique_ptr<renderer2d::SurfaceRGBA> frame);
    void clear_cache();

    std::shared_ptr<audio::AudioMixer> audio_mixer() { return m_audio_mixer; }
    void set_audio_mixer(std::shared_ptr<audio::AudioMixer> mixer) { m_audio_mixer = mixer; }

    ProxyManifest& proxy_manifest() { return m_proxy_manifest; }
    ProxyWorker& proxy_worker() { return *m_proxy_worker; }

    void set_use_proxies(bool use) { m_use_proxies = use; }
    bool use_proxies() const { return m_use_proxies; }

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
    std::map<std::string, std::shared_ptr<MeshAsset>> m_mesh_cache;
    std::unordered_map<std::string, std::shared_ptr<MediaAsset>> m_assets;
    
    MediaFallbackPolicy m_fallback_policy{MediaFallbackPolicy::UseProxy};
    std::shared_ptr<renderer2d::SurfaceRGBA> m_offline_placeholder;

    mutable std::mutex m_mutex;
    bool m_use_proxies{true};
    std::shared_ptr<audio::AudioMixer> m_audio_mixer;

    ProxyManifest m_proxy_manifest;
    std::unique_ptr<ProxyWorker> m_proxy_worker;
    PathResolver m_path_resolver;
};

} // namespace tachyon::media
