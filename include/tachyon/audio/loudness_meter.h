#pragma once

#include <vector>
#include <cstdint>

namespace tachyon::audio {

struct LoudnessMeasurement {
    float integrated_lufs{-70.0f};   // EBU R128 integrated
    float momentary_lufs{-70.0f};    // 400ms window
    float short_term_lufs{-70.0f};   // 3s window
    float true_peak_dbfs{-70.0f};
};

class LoudnessMeter {
public:
    LoudnessMeter();
    void process(const float* stereo_pcm, int nframes, int sample_rate);
    LoudnessMeasurement current() const;
    void reset();

    // K-weighting filter state (due biquad in cascata)
    struct BiquadState {
        double x1{0.0}, x2{0.0}, y1{0.0}, y2{0.0};
        double b0{1.0}, b1{0.0}, b2{0.0}, a1{0.0}, a2{0.0};
        double a0{1.0};
    };

private:
    
    BiquadState m_k_filter_stage1;
    BiquadState m_k_filter_stage2;
    
    // Blocco di integrazione 400ms e 3s
    std::vector<float> m_momentary_buffer;  // 400ms
    std::vector<float> m_short_term_buffer; // 3s
    std::vector<float> m_all_samples;       // per integrated
    
    int m_sample_rate{48000};
    double m_current_loudness{0.0};
    
    float apply_k_weighting(double sample);
    float calculate_loudness(const std::vector<float>& samples) const;
    float find_true_peak(const float* stereo_pcm, int nframes) const;
};

} // namespace tachyon::audio
