#pragma once

#include <vector>
#include <cstdint>
#include "tachyon/core/expressions/expression_engine.h"

namespace tachyon {
namespace audio {

// Audio waveform data for visualization
struct WaveformData {
    std::vector<float> min_samples;  // minimum amplitude per block
    std::vector<float> max_samples;  // maximum amplitude per block
    std::vector<float> rms_values;   // RMS amplitude per block
    int num_blocks = 0;
    double duration_seconds = 0.0;
    int sample_rate = 0;
    int channels = 0;
    float min_amplitude = -1.0f;
    float max_amplitude = 1.0f;
};

// Generate waveform from audio data
class WaveformGenerator {
public:
    struct WaveformSettings {
        int target_blocks = 1000;     // number of waveform blocks to generate
        int samples_per_block = 0;     // auto-calculate if 0
        bool compute_rms = true;        // compute RMS values
        float normalize_to = 1.0f;     // normalize amplitude to this value
    };
    
    // Static default settings instance
    static const WaveformSettings& default_settings() {
        static const WaveformSettings settings;
        return settings;
    }
    
    static WaveformData generate_from_interleaved(const float* samples,
                                                int num_samples,
                                                int channels,
                                                int sample_rate,
                                                const WaveformSettings& settings);
    
    static WaveformData generate_from_planar(const float** channel_data,
                                           int num_samples,
                                           int channels,
                                           int sample_rate,
                                           const WaveformSettings& settings);
};

// Audio reactive animation system
class AudioReactivityEngine {
public:
    struct BandLevels {
        float sub_bass = 0.0f;    // 20-60 Hz
        float bass = 0.0f;         // 60-250 Hz
        float low_mid = 0.0f;      // 250-500 Hz
        float mid = 0.0f;          // 500-2000 Hz
        float high_mid = 0.0f;     // 2000-4000 Hz
        float treble = 0.0f;       // 4000-20000 Hz
        float overall = 0.0f;      // full spectrum
    };
    
    struct ReactivitySettings {
        bool enable_sub_bass = true;
        bool enable_bass = true;
        bool enable_mid = true;
        bool enable_treble = true;
        float smoothing_factor = 0.8f; // 0=no smoothing, 1=max smoothing
        float sensitivity = 1.0f;      // multiplier for all levels
    };
    
    AudioReactivityEngine();
    ~AudioReactivityEngine();
    
    void set_settings(const ReactivitySettings& settings);
    ReactivitySettings get_settings() const;
    
    // Process audio block and return frequency band levels
    BandLevels process_audio_block(const float* interleaved_samples,
                                  int num_samples,
                                  int channels,
                                  int sample_rate);
    
    // Get smoothed levels (with temporal smoothing applied)
    BandLevels get_smoothed_levels() const;
    
    // Reset smoothing state
    void reset();

    // Convert BandLevels to AudioAnalysisData for expression engine
    static expressions::AudioAnalysisData to_analysis_data(const BandLevels& levels) {
        expressions::AudioAnalysisData data;
        data.bass = static_cast<double>(levels.bass);
        data.mid = static_cast<double>(levels.mid);
        data.treble = static_cast<double>(levels.treble);
        data.rms = static_cast<double>(levels.overall); // overall = full spectrum RMS
        data.beat = 0.0; // Beat detection not yet implemented
        return data;
    }
    
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

// Audio visualization helpers
namespace visualization {

// Generate waveform peaks for display at different zoom levels
std::vector<float> generate_mini_waveform(const WaveformData& waveform, int target_width);

// Color mapping for frequency bands
struct FrequencyBandColor {
    float r, g, b;
};

FrequencyBandColor get_band_color(const std::string& band_name);

} // namespace visualization

} // namespace audio
} // namespace tachyon
