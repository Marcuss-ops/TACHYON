#pragma once

#include "tachyon/core/spec/schema/objects/scene_spec.h"
#include "tachyon/core/expressions/expression_engine.h"

#include <filesystem>
#include <vector>

namespace tachyon::audio {

struct AudioBands {
    float bass{0.0f};
    float mid{0.0f};
    float high{0.0f};
    float presence{0.0f};
    float rms{0.0f};
};

struct BeatMap {
    std::vector<double> beat_times_seconds;  // tempo di ogni beat
    double bpm{0.0};
};

class AudioAnalyzer {
public:
    bool load(const std::filesystem::path& path, double sample_rate);
    AudioBands analyze_frame(double composition_time_seconds, double window_seconds = 1.0 / 30.0) const;
    
    // Beat detection: onset detection su banda bass
    BeatMap detect_beats(float onset_threshold = 0.15f) const;

    [[nodiscard]] double duration() const noexcept { return m_duration_seconds; }
    [[nodiscard]] double sample_rate() const noexcept { return m_sample_rate; }
    [[nodiscard]] bool loaded() const noexcept { return !m_samples.empty() && m_sample_rate > 0.0; }

    // Convert AudioBands to AudioAnalysisData for expression engine
    static expressions::AudioAnalysisData to_analysis_data(const AudioBands& bands);

private:
    std::vector<float> m_samples;
    double m_sample_rate{0.0};
    double m_duration_seconds{0.0};
};

} // namespace tachyon::audio
