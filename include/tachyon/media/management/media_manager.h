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
#include <filesystem>
#include <map>
#include <memory>
#include <mutex>
#include <string>

namespace tachyon::media {

enum class MediaFallbackPolicy {
    ReturnError,      // Fail if source missing
    ReturnOffline,    // Show "Offline" graphic
    UseProxy,         // Use proxy if original missing (default)
    UseOriginal       // Use original even if proxy requested
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
     * @brief Resolves the best available path for an asset.
     */
    std::filesystem::path resolve_media_path(const std::filesystem::path& path) const;

    /**
     * Acquires a VideoDecoder for the given path from a pool.
     * Must be returned via release_video_decoder.
     */
    VideoDecoder* acquire_video_decoder(const std::filesystem::path& path);
    void release_video_decoder(const std::filesystem::path& path, VideoDecoder* decoder);

    const MeshAsset* get_mesh(
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

private:
    struct VideoPool {
        std::vector<std::unique_ptr<VideoDecoder>> available;
        std::mutex mutex;
    };

    ImageManager m_image_manager;
    std::map<std::string, std::shared_ptr<VideoPool>> m_video_pools;
    std::map<std::string, std::unique_ptr<MeshAsset>> m_mesh_cache;
    std::unordered_map<std::string, std::shared_ptr<MediaAsset>> m_assets;
    
    MediaFallbackPolicy m_fallback_policy{MediaFallbackPolicy::UseProxy};
    std::shared_ptr<renderer2d::SurfaceRGBA> m_offline_placeholder;

    mutable std::mutex m_mutex;
    bool m_use_proxies{true};
    std::shared_ptr<audio::AudioMixer> m_audio_mixer;

    ProxyManifest m_proxy_manifest;
    std::unique_ptr<ProxyWorker> m_proxy_worker;
};

} // namespace tachyon::media
