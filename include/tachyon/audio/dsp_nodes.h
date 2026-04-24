#pragma once

#include "tachyon/audio/audio_graph.h"
#include <algorithm>
#include <cmath>

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

// ---------------------------------------------------------------------------
// CompressorNode – sidechain ducker for audio ducking
// ---------------------------------------------------------------------------
class CompressorNode : public AudioNode {
public:
    explicit CompressorNode(const DuckerConfig& config = DuckerConfig())
        : m_config(config),
          m_current_gain(1.0f),
          m_envelope(0.0f) {
        // Precompute attack/release coefficients based on sample rate
        const float sample_rate = static_cast<float>(kDSPSampleRate);
        const float attack_samples = m_config.attack_ms * 0.001f * sample_rate;
        const float release_samples = m_config.release_ms * 0.001f * sample_rate;
        m_attack_coeff = std::exp(-1.0f / attack_samples);
        m_release_coeff = std::exp(-1.0f / release_samples);
        m_threshold_linear = std::pow(10.0f, m_config.threshold_db / 20.0f);
    }

    // Set the sidechain audio buffer (interleaved stereo, same nframes as main process)
    void set_sidechain(const float* sidechain, int nframes) {
        m_sidechain = sidechain;
        m_sidechain_nframes = nframes;
    }

    void process(float* io, int nframes) override {
        if (!m_sidechain || m_sidechain_nframes < nframes) {
            // No sidechain or insufficient sidechain data: no compression
            return;
        }

        const int total_samples = nframes * 2; // interleaved stereo
        for (int i = 0; i < total_samples; ++i) {
            // Compute sidechain peak for this sample
            float sidechain_sample = std::abs(m_sidechain[i]) * m_config.sidechain_gain;
            
            // Update envelope follower (peak-based)
            if (sidechain_sample > m_envelope) {
                m_envelope = m_attack_coeff * m_envelope + (1.0f - m_attack_coeff) * sidechain_sample;
            } else {
                m_envelope = m_release_coeff * m_envelope + (1.0f - m_release_coeff) * sidechain_sample;
            }

            // Compute gain reduction if envelope exceeds threshold
            float target_gain = 1.0f;
            if (m_envelope > m_threshold_linear) {
                float envelope_db = 20.0f * std::log10(m_envelope);
                float gain_reduction_db = (envelope_db - m_config.threshold_db) * (1.0f - 1.0f / m_config.ratio);
                target_gain = std::pow(10.0f, -gain_reduction_db / 20.0f);
            }

            // Smooth gain changes to avoid clicks
            m_current_gain = 0.9f * m_current_gain + 0.1f * target_gain;

            // Apply gain to main audio (music bus)
            io[i] *= m_current_gain;
        }

        // Reset sidechain after processing to avoid stale data
        m_sidechain = nullptr;
        m_sidechain_nframes = 0;
    }

    std::string name() const override { return "Compressor (Ducker)"; }

    void reset() override {
        m_current_gain = 1.0f;
        m_envelope = 0.0f;
        m_sidechain = nullptr;
        m_sidechain_nframes = 0;
    }

private:
    DuckerConfig m_config;
    float m_threshold_linear;
    float m_attack_coeff;
    float m_release_coeff;
    float m_current_gain;
    float m_envelope;
    const float* m_sidechain = nullptr;
    int m_sidechain_nframes = 0;
};

} // namespace tachyon::audio
