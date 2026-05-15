#pragma once

#include "tachyon/core/media/media_interfaces.h"
#include "tachyon/core/media/clip_processor.h"
#include "tachyon/core/media/overlay_merger.h"
#include "tachyon/core/media/audio_extract.h"
#include "tachyon/core/media/video_concat.h"
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
    void register_transition_renderer(const std::string& name, std::function<std::unique_ptr<core::media::ITransitionRenderer>()> factory);
    void register_clip_processor(const std::string& name, std::function<std::unique_ptr<core::media::IClipProcessor>()> factory);
    void register_overlay_merger(const std::string& name, std::function<std::unique_ptr<core::media::IOverlayMerger>()> factory);
    void register_audio_extractor(const std::string& name, std::function<std::unique_ptr<core::media::IAudioExtractor>()> factory);
    void register_video_concat(const std::string& name, std::function<std::unique_ptr<core::media::IVideoConcat>()> factory);

    // Capability getters
    std::unique_ptr<core::media::IMediaProbe> create_probe(const std::string& name = "default") const;
    std::unique_ptr<core::media::IAudioAnalyzer> create_audio_analyzer(const std::string& name = "default") const;
    std::unique_ptr<core::media::IVideoEncoder> create_video_encoder(const std::string& name = "default") const;
    std::unique_ptr<core::media::ITransitionRenderer> create_transition_renderer(const std::string& name = "default") const;
    std::unique_ptr<core::media::IClipProcessor> create_clip_processor(const std::string& name = "default") const;
    std::unique_ptr<core::media::IOverlayMerger> create_overlay_merger(const std::string& name = "default") const;
    std::unique_ptr<core::media::IAudioExtractor> create_audio_extractor(const std::string& name = "default") const;
    std::unique_ptr<core::media::IVideoConcat> create_video_concat(const std::string& name = "default") const;

    // Default management
    void set_default_probe(const std::string& name) { m_default_probe = name; }
    void set_default_audio_analyzer(const std::string& name) { m_default_audio_analyzer = name; }
    void set_default_video_encoder(const std::string& name) { m_default_video_encoder = name; }
    void set_default_transition_renderer(const std::string& name) { m_default_transition_renderer = name; }

    // List available capabilities
    std::vector<std::string> list_probes() const;
    std::vector<std::string> list_audio_analyzers() const;
    std::vector<std::string> list_video_encoders() const;
    std::vector<std::string> list_transition_renderers() const;

private:
    BackendRegistry() = default;

    std::string m_default_probe = "ffmpeg";
    std::string m_default_audio_analyzer = "whisper";
    std::string m_default_video_encoder = "ffmpeg";
    std::string m_default_transition_renderer = "builtin";

    std::unordered_map<std::string, std::function<std::unique_ptr<core::media::IMediaProbe>()>> m_probes;
    std::unordered_map<std::string, std::function<std::unique_ptr<core::media::IAudioAnalyzer>()>> m_audio_analyzers;
    std::unordered_map<std::string, std::function<std::unique_ptr<core::media::IVideoEncoder>()>> m_video_encoders;
    std::unordered_map<std::string, std::function<std::unique_ptr<core::media::ITransitionRenderer>()>> m_transition_renderers;
    std::unordered_map<std::string, std::function<std::unique_ptr<core::media::IClipProcessor>()>> m_clip_processors;
    std::unordered_map<std::string, std::function<std::unique_ptr<core::media::IOverlayMerger>()>> m_overlay_mergers;
    std::unordered_map<std::string, std::function<std::unique_ptr<core::media::IAudioExtractor>()>> m_audio_extractors;
    std::unordered_map<std::string, std::function<std::unique_ptr<core::media::IVideoConcat>()>> m_video_concats;
};

} // namespace tachyon::backends
