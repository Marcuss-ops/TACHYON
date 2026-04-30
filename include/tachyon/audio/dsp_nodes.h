#pragma once

#include "tachyon/audio/audio_graph.h"
#include <algorithm>
#include <cmath>
#include <string>

namespace tachyon::audio {

// ---------------------------------------------------------------------------
// GainNode – simple volume multiplier
// ---------------------------------------------------------------------------
class GainNode : public AudioNode {
public:
    explicit GainNode(float initial_gain = 1.0f) : m_gain(initial_gain) {}

    void set_gain(float g) { m_gain = g; }
    float gain() const { return m_gain; }

    void process(float* io, int nframes) override {
        for (int i = 0; i < nframes * 2; ++i) {
            io[i] *= m_gain;
        }
    }

    std::string name() const override { return "Gain"; }

private:
    float m_gain;
};

// ---------------------------------------------------------------------------
// PanNode – stereo panner (linear/equal-power)
// ---------------------------------------------------------------------------
class PanNode : public AudioNode {
public:
    // pan: -1.0 (left) to 1.0 (right), 0.0 (center)
    explicit PanNode(float p = 0.0f) : m_pan(p) {}

    void set_pan(float p) { m_pan = std::clamp(p, -1.0f, 1.0f); }
    float pan() const { return m_pan; }

    void process(float* io, int nframes) override {
        // Equal power panning (approximate)
        constexpr float kPi = 3.14159265358979323846f;
        float theta = (m_pan + 1.0f) * (kPi / 4.0f);
        float left_gain = std::cos(theta);
        float right_gain = std::sin(theta);

        for (int i = 0; i < nframes; ++i) {
            io[i*2]   *= left_gain;
            io[i*2+1] *= right_gain;
        }
    }

private:
    float m_pan;
};

// ---------------------------------------------------------------------------
// LimiterNode – hard ceiling to prevent clipping
// ---------------------------------------------------------------------------
class LimiterNode : public AudioNode {
public:
    explicit LimiterNode(float ceiling = 1.0f) : m_ceiling(ceiling) {}

    void process(float* io, int nframes) override {
        for (int i = 0; i < nframes * 2; ++i) {
            io[i] = std::clamp(io[i], -m_ceiling, m_ceiling);
        }
    }

    std::string name() const override { return "Limiter"; }

private:
    float m_ceiling;
};

// ---------------------------------------------------------------------------
// FadeNode – fade in/out and trim for audio regions
// ---------------------------------------------------------------------------
class FadeNode : public AudioNode {
public:
    enum class FadeType { Linear, Exponential };

    FadeNode() = default;

    void set_fade_in(float duration_samples, FadeType type = FadeType::Linear) {
        m_fade_in_samples = duration_samples;
        m_fade_in_type = type;
    }

    void set_fade_out(float duration_samples, FadeType type = FadeType::Linear) {
        m_fade_out_samples = duration_samples;
        m_fade_out_type = type;
    }

    void set_trim(float start_sample, float end_sample) {
        m_trim_start = start_sample;
        m_trim_end = end_sample;
    }

    void process(float* io, int nframes) override {
        const int total_samples = nframes * 2;
        for (int i = 0; i < total_samples; i += 2) {
            float sample_index = i / 2 + m_processed_samples;

            if (sample_index < m_trim_start || (m_trim_end > 0 && sample_index >= m_trim_end)) {
                io[i] = io[i + 1] = 0.0f;
                continue;
            }

            float gain = 1.0f;
            float local_index = sample_index - m_trim_start;

            if (m_fade_in_samples > 0 && local_index < m_fade_in_samples) {
                float t = local_index / m_fade_in_samples;
                gain *= (m_fade_in_type == FadeType::Linear) ? t : t * t;
            }

            if (m_fade_out_samples > 0 && m_trim_end > m_trim_start) {
                float remaining = m_trim_end - sample_index;
                if (remaining < m_fade_out_samples && remaining > 0) {
                    float t = remaining / m_fade_out_samples;
                    gain *= (m_fade_out_type == FadeType::Linear) ? t : t * t;
                }
            }

            io[i] *= gain;
            io[i + 1] *= gain;
        }
        m_processed_samples += static_cast<float>(nframes);
    }

    void reset() override { m_processed_samples = 0; }
    std::string name() const override { return "Fade"; }

private:
    float m_fade_in_samples = 0.0f;
    float m_fade_out_samples = 0.0f;
    float m_trim_start = 0.0f;
    float m_trim_end = 0.0f;
    FadeType m_fade_in_type = FadeType::Linear;
    FadeType m_fade_out_type = FadeType::Linear;
    float m_processed_samples = 0.0f;
};

// ---------------------------------------------------------------------------
// CompressorNode – sidechain ducker for audio ducking
// ---------------------------------------------------------------------------
struct DuckerConfig {
    float threshold_db{-20.0f};      // dB, sidechain threshold
    float ratio{4.0f};              // compression ratio (4:1)
    float attack_ms{10.0f};         // attack time in ms
    float release_ms{200.0f};       // release time in ms
    float sidechain_gain{1.0f};     // gain applied to sidechain input
};

struct AudioTrackMixParams {
  double start_offset_seconds{0.0};
  float volume{1.0f};
  float pan{0.0f};            // -1.0 (L) to 1.0 (R)
  float playback_speed{1.0f}; // 1.0 = normal, 0.5 = slow, 2.0 = fast
  float pitch_shift{1.0f};    // 1.0 = normal, 0.5 = octave down, 2.0 = octave up
  bool pitch_correct{false};  // true = preserve pitch when speed changes (WSOLA)
  bool enabled{true};
  bool is_sidechain_source{false};  // true = voiceover/narration (provides sidechain signal)
  DuckerConfig duck_config{};       // ducking config (used when this track ducks others)
};

class CompressorNode : public AudioNode {
public:
    static constexpr int kDSPSampleRate = 48000;

    explicit CompressorNode(const DuckerConfig& config = DuckerConfig())
        : m_config(config),
          m_current_gain(1.0f),
          m_envelope(0.0f) {
        const float sample_rate = static_cast<float>(kDSPSampleRate);
        const float attack_samples = m_config.attack_ms * 0.001f * sample_rate;
        const float release_samples = m_config.release_ms * 0.001f * sample_rate;
        m_attack_coeff = std::exp(-1.0f / attack_samples);
        m_release_coeff = std::exp(-1.0f / release_samples);
        m_threshold_linear = std::pow(10.0f, m_config.threshold_db / 20.0f);
    }

    void set_sidechain(const float* sidechain, int nframes) {
        m_sidechain = sidechain;
        m_sidechain_nframes = nframes;
    }

    void process(float* io, int nframes) override {
        if (!m_sidechain || m_sidechain_nframes < nframes) return;

        const int total_samples = nframes * 2;
        for (int i = 0; i < total_samples; ++i) {
            float sidechain_sample = std::abs(m_sidechain[i]) * m_config.sidechain_gain;
            
            if (sidechain_sample > m_envelope) {
                m_envelope = m_attack_coeff * m_envelope + (1.0f - m_attack_coeff) * sidechain_sample;
            } else {
                m_envelope = m_release_coeff * m_envelope + (1.0f - m_release_coeff) * sidechain_sample;
            }

            float target_gain = 1.0f;
            if (m_envelope > m_threshold_linear) {
                float envelope_db = 20.0f * std::log10(m_envelope);
                float gain_reduction_db = (envelope_db - m_config.threshold_db) * (1.0f - 1.0f / m_config.ratio);
                target_gain = std::pow(10.0f, -gain_reduction_db / 20.0f);
            }

            m_current_gain = 0.9f * m_current_gain + 0.1f * target_gain;
            io[i] *= m_current_gain;
        }

        m_sidechain = nullptr;
        m_sidechain_nframes = 0;
    }

    void reset() override {
        m_current_gain = 1.0f;
        m_envelope = 0.0f;
        m_sidechain = nullptr;
        m_sidechain_nframes = 0;
    }

    std::string name() const override { return "Compressor (Ducker)"; }

private:
    DuckerConfig m_config;
    float m_current_gain;
    float m_envelope;
    float m_attack_coeff{0.0f};
    float m_release_coeff{0.0f};
    float m_threshold_linear{0.0f};
    const float* m_sidechain{nullptr};
    int m_sidechain_nframes{0};
};

// ---------------------------------------------------------------------------
// LowPassNode – 2nd order Butterworth low-pass filter
// ---------------------------------------------------------------------------
class LowPassNode : public AudioNode {
public:
    LowPassNode(float cutoff_hz = 1000.0f, float sample_rate = 48000.0f) {
        set_cutoff(cutoff_hz, sample_rate);
    }

    void set_cutoff(float cutoff_hz, float sample_rate = 48000.0f) {
        const float w0 = 2.0f * 3.14159265358979323846f * cutoff_hz / sample_rate;
        const float cos_w0 = std::cos(w0);
        const float alpha = std::sin(w0) / (2.0f * 0.7071067811865476f);

        const float a0 = 1.0f + alpha;
        m_b0 = (1.0f - cos_w0) / 2.0f / a0;
        m_b1 = (1.0f - cos_w0) / a0;
        m_b2 = (1.0f - cos_w0) / 2.0f / a0;
        m_a1 = -2.0f * cos_w0 / a0;
        m_a2 = (1.0f - alpha) / a0;

        reset();
    }

    void process(float* io, int nframes) override {
        for (int i = 0; i < nframes * 2; ++i) {
            const float input = io[i];
            const float output = m_b0 * input + m_b1 * m_x1 + m_b2 * m_x2 - m_a1 * m_y1 - m_a2 * m_y2;
            m_x2 = m_x1;
            m_x1 = input;
            m_y2 = m_y1;
            m_y1 = output;
            io[i] = output;
        }
    }

    void reset() override {
        m_x1 = m_x2 = 0.0f;
        m_y1 = m_y2 = 0.0f;
    }

    std::string name() const override { return "LowPass"; }

private:
    float m_b0 = 0.0f, m_b1 = 0.0f, m_b2 = 0.0f;
    float m_a1 = 0.0f, m_a2 = 0.0f;
    float m_x1 = 0.0f, m_x2 = 0.0f;
    float m_y1 = 0.0f, m_y2 = 0.0f;
};

// ---------------------------------------------------------------------------
// HighPassNode – 2nd order Butterworth high-pass filter
// ---------------------------------------------------------------------------
class HighPassNode : public AudioNode {
public:
    HighPassNode(float cutoff_hz = 1000.0f, float sample_rate = 48000.0f) {
        set_cutoff(cutoff_hz, sample_rate);
    }

    void set_cutoff(float cutoff_hz, float sample_rate = 48000.0f) {
        const float w0 = 2.0f * 3.14159265358979323846f * cutoff_hz / sample_rate;
        const float cos_w0 = std::cos(w0);
        const float alpha = std::sin(w0) / (2.0f * 0.7071067811865476f);

        const float a0 = 1.0f + alpha;
        m_b0 = (1.0f + cos_w0) / 2.0f / a0;
        m_b1 = -(1.0f + cos_w0) / a0;
        m_b2 = (1.0f + cos_w0) / 2.0f / a0;
        m_a1 = -2.0f * cos_w0 / a0;
        m_a2 = (1.0f - alpha) / a0;

        reset();
    }

    void process(float* io, int nframes) override {
        for (int i = 0; i < nframes * 2; ++i) {
            const float input = io[i];
            const float output = m_b0 * input + m_b1 * m_x1 + m_b2 * m_x2 - m_a1 * m_y1 - m_a2 * m_y2;
            m_x2 = m_x1;
            m_x1 = input;
            m_y2 = m_y1;
            m_y1 = output;
            io[i] = output;
        }
    }

    void reset() override {
        m_x1 = m_x2 = 0.0f;
        m_y1 = m_y2 = 0.0f;
    }

    std::string name() const override { return "HighPass"; }

private:
    float m_b0 = 0.0f, m_b1 = 0.0f, m_b2 = 0.0f;
    float m_a1 = 0.0f, m_a2 = 0.0f;
    float m_x1 = 0.0f, m_x2 = 0.0f;
    float m_y1 = 0.0f, m_y2 = 0.0f;
};

} // namespace tachyon::audio
