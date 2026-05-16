#include "tachyon/backends/backend_registry.h"
#include <algorithm>

namespace tachyon::backends {

BackendRegistry& BackendRegistry::instance() {
    static BackendRegistry reg;
    return reg;
}

void BackendRegistry::register_probe(const std::string& name, std::function<std::unique_ptr<core::media::IMediaProbe>()> factory) {
    m_probes[name] = std::move(factory);
}

void BackendRegistry::register_audio_analyzer(const std::string& name, std::function<std::unique_ptr<core::media::IAudioAnalyzer>()> factory) {
    m_audio_analyzers[name] = std::move(factory);
}

void BackendRegistry::register_video_encoder(const std::string& name, std::function<std::unique_ptr<core::media::IVideoEncoder>()> factory) {
    m_video_encoders[name] = std::move(factory);
}

void BackendRegistry::register_transition_renderer(const std::string& name, std::function<std::unique_ptr<core::media::ITransitionRenderer>()> factory) {
    m_transition_renderers[name] = std::move(factory);
}

void BackendRegistry::register_clip_processor(const std::string& name, std::function<std::unique_ptr<core::media::IClipProcessor>()> factory) {
    m_clip_processors[name] = std::move(factory);
}

void BackendRegistry::register_overlay_merger(const std::string& name, std::function<std::unique_ptr<core::media::IOverlayMerger>()> factory) {
    m_overlay_mergers[name] = std::move(factory);
}

void BackendRegistry::register_audio_extractor(const std::string& name, std::function<std::unique_ptr<core::media::IAudioExtractor>()> factory) {
    m_audio_extractors[name] = std::move(factory);
}

void BackendRegistry::register_video_concat(const std::string& name, std::function<std::unique_ptr<core::media::IVideoConcat>()> factory) {
    m_video_concats[name] = std::move(factory);
}

std::unique_ptr<core::media::IMediaProbe> BackendRegistry::create_probe(const std::string& name) const {
    const std::string& target = (name == "default") ? m_default_probe : name;
    auto it = m_probes.find(target);
    if (it != m_probes.end()) return it->second();
    return nullptr;
}

std::unique_ptr<core::media::IAudioAnalyzer> BackendRegistry::create_audio_analyzer(const std::string& name) const {
    const std::string& target = (name == "default") ? m_default_audio_analyzer : name;
    auto it = m_audio_analyzers.find(target);
    if (it != m_audio_analyzers.end()) return it->second();
    return nullptr;
}

std::unique_ptr<core::media::IVideoEncoder> BackendRegistry::create_video_encoder(const std::string& name) const {
    const std::string& target = (name == "default") ? m_default_video_encoder : name;
    auto it = m_video_encoders.find(target);
    if (it != m_video_encoders.end()) return it->second();
    return nullptr;
}

std::unique_ptr<core::media::ITransitionRenderer> BackendRegistry::create_transition_renderer(const std::string& name) const {
    const std::string& target = (name == "default") ? m_default_transition_renderer : name;
    auto it = m_transition_renderers.find(target);
    if (it != m_transition_renderers.end()) return it->second();
    return nullptr;
}

std::unique_ptr<core::media::IClipProcessor> BackendRegistry::create_clip_processor(const std::string& name) const {
    auto it = m_clip_processors.find(name == "default" ? "ffmpeg" : name);
    if (it != m_clip_processors.end()) return it->second();
    return nullptr;
}

std::unique_ptr<core::media::IOverlayMerger> BackendRegistry::create_overlay_merger(const std::string& name) const {
    auto it = m_overlay_mergers.find(name == "default" ? "ffmpeg" : name);
    if (it != m_overlay_mergers.end()) return it->second();
    return nullptr;
}

std::unique_ptr<core::media::IAudioExtractor> BackendRegistry::create_audio_extractor(const std::string& name) const {
    auto it = m_audio_extractors.find(name == "default" ? "ffmpeg" : name);
    if (it != m_audio_extractors.end()) return it->second();
    return nullptr;
}

std::unique_ptr<core::media::IVideoConcat> BackendRegistry::create_video_concat(const std::string& name) const {
    auto it = m_video_concats.find(name == "default" ? "ffmpeg" : name);
    if (it != m_video_concats.end()) return it->second();
    return nullptr;
}

std::vector<std::string> BackendRegistry::list_probes() const {
    std::vector<std::string> names;
    for (const auto& [name, _] : m_probes) names.push_back(name);
    std::sort(names.begin(), names.end());
    return names;
}

std::vector<std::string> BackendRegistry::list_audio_analyzers() const {
    std::vector<std::string> names;
    for (const auto& [name, _] : m_audio_analyzers) names.push_back(name);
    std::sort(names.begin(), names.end());
    return names;
}

std::vector<std::string> BackendRegistry::list_video_encoders() const {
    std::vector<std::string> names;
    for (const auto& [name, _] : m_video_encoders) names.push_back(name);
    std::sort(names.begin(), names.end());
    return names;
}

std::vector<std::string> BackendRegistry::list_transition_renderers() const {
    std::vector<std::string> names;
    for (const auto& [name, _] : m_transition_renderers) names.push_back(name);
    std::sort(names.begin(), names.end());
    return names;
}

} // namespace tachyon::backends
