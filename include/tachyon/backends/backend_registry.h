#pragma once

#include "tachyon/core/media/media_interfaces.h"
#include <string>
#include <memory>
#include <functional>
#include <unordered_map>

namespace tachyon::backends {

/**
 * @brief Registry for engine backends and capabilities.
 * 
 * Allows decoupling the engine core from concrete implementations like FFmpeg or Whisper.
 */
class BackendRegistry {
public:
    static BackendRegistry& instance();

    // Capability setters
    void register_probe(const std::string& name, std::function<std::unique_ptr<core::media::IMediaProbe>()> factory);
    void register_audio_analyzer(const std::string& name, std::function<std::unique_ptr<core::media::IAudioAnalyzer>()> factory);
    void register_video_encoder(const std::string& name, std::function<std::unique_ptr<core::media::IVideoEncoder>()> factory);

    // Capability getters
    std::unique_ptr<core::media::IMediaProbe> create_probe(const std::string& name = "default") const;
    std::unique_ptr<core::media::IAudioAnalyzer> create_audio_analyzer(const std::string& name = "default") const;
    std::unique_ptr<core::media::IVideoEncoder> create_video_encoder(const std::string& name = "default") const;

    // List available capabilities
    std::vector<std::string> list_probes() const;
    std::vector<std::string> list_audio_analyzers() const;
    std::vector<std::string> list_video_encoders() const;

private:
    BackendRegistry() = default;

    std::unordered_map<std::string, std::function<std::unique_ptr<core::media::IMediaProbe>()>> m_probes;
    std::unordered_map<std::string, std::function<std::unique_ptr<core::media::IAudioAnalyzer>()>> m_audio_analyzers;
    std::unordered_map<std::string, std::function<std::unique_ptr<core::media::IVideoEncoder>()>> m_video_encoders;
};

} // namespace tachyon::backends
