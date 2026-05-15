#include "tachyon/backends/backend_registry.h"

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

std::unique_ptr<core::media::IMediaProbe> BackendRegistry::create_probe(const std::string& name) const {
    auto it = m_probes.find(name == "default" && !m_probes.empty() ? m_probes.begin()->first : name);
    if (it != m_probes.end()) return it->second();
    return nullptr;
}

std::unique_ptr<core::media::IAudioAnalyzer> BackendRegistry::create_audio_analyzer(const std::string& name) const {
    auto it = m_audio_analyzers.find(name == "default" && !m_audio_analyzers.empty() ? m_audio_analyzers.begin()->first : name);
    if (it != m_audio_analyzers.end()) return it->second();
    return nullptr;
}

std::unique_ptr<core::media::IVideoEncoder> BackendRegistry::create_video_encoder(const std::string& name) const {
    auto it = m_video_encoders.find(name == "default" && !m_video_encoders.empty() ? m_video_encoders.begin()->first : name);
    if (it != m_video_encoders.end()) return it->second();
    return nullptr;
}

std::vector<std::string> BackendRegistry::list_probes() const {
    std::vector<std::string> names;
    for (const auto& [name, _] : m_probes) names.push_back(name);
    return names;
}

std::vector<std::string> BackendRegistry::list_audio_analyzers() const {
    std::vector<std::string> names;
    for (const auto& [name, _] : m_audio_analyzers) names.push_back(name);
    return names;
}

std::vector<std::string> BackendRegistry::list_video_encoders() const {
    std::vector<std::string> names;
    for (const auto& [name, _] : m_video_encoders) names.push_back(name);
    return names;
}

} // namespace tachyon::backends
