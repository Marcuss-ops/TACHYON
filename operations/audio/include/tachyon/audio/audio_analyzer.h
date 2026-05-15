#pragma once

#include "tachyon/core/audio/audio_interfaces.h"
#include "tachyon/core/expressions/expression_engine.h"
#include <filesystem>
#include <vector>

namespace tachyon::audio {

class AudioAnalyzer : public IAudioAnalyzer {
public:
    bool load(const std::filesystem::path& path, double sample_rate);
    
    AudioBands analyze_frame(double composition_time_seconds, double window_seconds = 1.0 / 30.0) const override;
    BeatMap detect_beats(float onset_threshold = 0.15f) const override;

    [[nodiscard]] double duration() const noexcept override { return m_duration_seconds; }
    [[nodiscard]] double sample_rate() const noexcept override { return m_sample_rate; }
    [[nodiscard]] bool loaded() const noexcept override { return !m_samples.empty() && m_sample_rate > 0.0; }

    // Convert AudioBands to AudioAnalysisData for expression engine
    static expressions::AudioAnalysisData to_analysis_data(const AudioBands& bands);

private:
    std::vector<float> m_samples;
    double m_sample_rate{0.0};
    double m_duration_seconds{0.0};
};

} // namespace tachyon::audio
