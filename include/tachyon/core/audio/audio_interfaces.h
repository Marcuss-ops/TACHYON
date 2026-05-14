#pragma once

#include "tachyon/core/audio/audio_types.h"
#include <vector>

namespace tachyon::audio {

struct BeatMap {
    std::vector<double> beat_times_seconds;
    double bpm{0.0};
};

/**
 * @brief Interface for audio analysis used during scene evaluation.
 */
class IAudioAnalyzer {
public:
    virtual ~IAudioAnalyzer() = default;
    virtual AudioBands analyze_frame(double composition_time_seconds, double window_seconds = 1.0 / 30.0) const = 0;
    virtual BeatMap detect_beats(float onset_threshold = 0.15f) const = 0;
    virtual double duration() const noexcept = 0;
    virtual double sample_rate() const noexcept = 0;
    virtual bool loaded() const noexcept = 0;
};

} // namespace tachyon::audio
