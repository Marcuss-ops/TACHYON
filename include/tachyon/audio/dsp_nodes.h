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

} // namespace tachyon::audio
