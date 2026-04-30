#include "tachyon/audio/loudness_meter.h"

#include <algorithm>
#include <cmath>
#include <numeric>

namespace tachyon::audio {
namespace {

// EBU R128 K-weighting filter coefficients
// Stage 1: High-pass filter @ 150Hz
void setup_k_filter_stage1(LoudnessMeter::BiquadState& state, double sample_rate) {
    const double f0 = 150.0;
    const double w0 = 2.0 * std::acos(-1.0) * f0 / sample_rate;
    const double cos_w0 = std::cos(w0);
    const double sin_w0 = std::sin(w0);
    const double alpha = sin_w0 / 2.0;
    
    state.b0 = 1.0 + cos_w0;
    state.b1 = -2.0 * (1.0 + cos_w0);
    state.b2 = 1.0 + cos_w0;
    state.a0 = 1.0 + alpha;
    state.a1 = -2.0 * cos_w0;
    state.a2 = 1.0 - alpha;
    
    // Normalize
    state.b0 /= state.a0;
    state.b1 /= state.a0;
    state.b2 /= state.a0;
    state.a1 /= state.a0;
    state.a2 /= state.a0;
}

// Stage 2: High-shelf filter @ 3.5kHz with 4dB gain
void setup_k_filter_stage2(LoudnessMeter::BiquadState& state, double sample_rate) {
    const double f0 = 3500.0;
    const double gain_db = 4.0;
    const double A = std::pow(10.0, gain_db / 40.0);
    const double w0 = 2.0 * std::acos(-1.0) * f0 / sample_rate;
    const double cos_w0 = std::cos(w0);
    const double sin_w0 = std::sin(w0);
    const double alpha = sin_w0 / 2.0 * std::sqrt((A + 1.0/A) * (1.0/1.0 - 1.0) + 2.0);
    
    state.b0 = A * ((A + 1.0) - (A - 1.0) * cos_w0 + 2.0 * std::sqrt(A) * alpha);
    state.b1 = 2.0 * A * ((A - 1.0) - (A + 1.0) * cos_w0);
    state.b2 = A * ((A + 1.0) - (A - 1.0) * cos_w0 - 2.0 * std::sqrt(A) * alpha);
    state.a0 = (A + 1.0) + (A - 1.0) * cos_w0 + 2.0 * std::sqrt(A) * alpha;
    state.a1 = -2.0 * ((A - 1.0) + (A + 1.0) * cos_w0);
    state.a2 = (A + 1.0) + (A - 1.0) * cos_w0 - 2.0 * std::sqrt(A) * alpha;
    
    // Normalize
    state.b0 /= state.a0;
    state.b1 /= state.a0;
    state.b2 /= state.a0;
    state.a1 /= state.a0;
    state.a2 /= state.a0;
}

double apply_biquad(const LoudnessMeter::BiquadState& state, double input, 
                    double& x1, double& x2, double& y1, double& y2) {
    const double output = state.b0 * input + state.b1 * x1 + state.b2 * x2 
                         - state.a1 * y1 - state.a2 * y2;
    x2 = x1;
    x1 = input;
    y2 = y1;
    y1 = output;
    return output;
}

} // namespace

LoudnessMeter::LoudnessMeter() {
    reset();
}

void LoudnessMeter::reset() {
    m_k_filter_stage1 = BiquadState{};
    m_k_filter_stage2 = BiquadState{};
    m_momentary_buffer.clear();
    m_short_term_buffer.clear();
    m_all_samples.clear();
    m_current_loudness = 0.0;
    m_sample_rate = 48000;
    m_true_peak_dbfs = -70.0f;
}

void LoudnessMeter::process(const float* stereo_pcm, int nframes, int sample_rate) {
    if (!stereo_pcm || nframes <= 0) {
        return;
    }
    
    m_sample_rate = sample_rate;
    
    // Setup filtri se sample_rate cambia
    static bool initialized = false;
    static int last_sr = 0;
    if (!initialized || last_sr != sample_rate) {
        setup_k_filter_stage1(m_k_filter_stage1, sample_rate);
        setup_k_filter_stage2(m_k_filter_stage2, sample_rate);
        initialized = true;
        last_sr = sample_rate;
    }
    
    // Dimensione buffer per 400ms e 3s
    const int momentary_samples = static_cast<int>(0.4 * sample_rate);
    const int short_term_samples = static_cast<int>(3.0 * sample_rate);
    
    // Processa ogni campione stereo -> mono -> K-weighted
    for (int i = 0; i < nframes; ++i) {
        // Downmix stereo a mono
        const float left = stereo_pcm[i * 2];
        const float right = stereo_pcm[i * 2 + 1];
        const float mono = (left + right) * 0.5f;
        
        // Applica K-weighting filter (due stadi in cascata)
        double weighted = apply_k_weighting(static_cast<double>(mono));
        
        // Quadrato del segnale per calcolo energia
        const float energy = static_cast<float>(weighted * weighted);
        
        // Aggiungi ai buffer
        m_momentary_buffer.push_back(energy);
        m_short_term_buffer.push_back(energy);
        m_all_samples.push_back(energy);
        
        // Mantieni dimensioni buffer fisse
        if (static_cast<int>(m_momentary_buffer.size()) > momentary_samples) {
            m_momentary_buffer.erase(m_momentary_buffer.begin());
        }
        if (static_cast<int>(m_short_term_buffer.size()) > short_term_samples) {
            m_short_term_buffer.erase(m_short_term_buffer.begin());
        }
    }
    
    // Aggiorna true peak massimo
    float current_peak = find_true_peak(stereo_pcm, nframes);
    if (current_peak > m_true_peak_dbfs) {
        m_true_peak_dbfs = current_peak;
    }
}

float LoudnessMeter::apply_k_weighting(double sample) {
    // Stage 1: High-pass @ 150Hz
    double output = apply_biquad(m_k_filter_stage1, sample,
                                  m_k_filter_stage1.x1, m_k_filter_stage1.x2,
                                  m_k_filter_stage1.y1, m_k_filter_stage1.y2);
    
    // Stage 2: High-shelf @ 3.5kHz +4dB
    output = apply_biquad(m_k_filter_stage2, output,
                          m_k_filter_stage2.x1, m_k_filter_stage2.x2,
                          m_k_filter_stage2.y1, m_k_filter_stage2.y2);
    
    return static_cast<float>(output);
}

float LoudnessMeter::calculate_loudness(const std::vector<float>& samples) const {
    if (samples.empty()) {
        return -70.0f;
    }
    
    // Calcola media dell'energia
    const double sum = std::accumulate(samples.begin(), samples.end(), 0.0);
    const double mean_energy = sum / static_cast<double>(samples.size());
    
    // Converte in LUFS: -0.691 + 10 * log10(mean_energy)
    if (mean_energy <= 1e-12) {
        return -70.0f;
    }
    
    return static_cast<float>(-0.691 + 10.0 * std::log10(mean_energy));
}

float LoudnessMeter::find_true_peak(const float* stereo_pcm, int nframes) const {
    if (!stereo_pcm || nframes <= 0) {
        return -70.0f;
    }
    
    float max_peak = 0.0f;
    for (int i = 0; i < nframes; ++i) {
        const float left = std::abs(stereo_pcm[i * 2]);
        const float right = std::abs(stereo_pcm[i * 2 + 1]);
        max_peak = std::max({max_peak, left, right});
    }
    
    if (max_peak <= 1e-12f) {
        return -70.0f;
    }
    
    // Converte in dBFS
    return static_cast<float>(20.0 * std::log10(max_peak));
}

LoudnessMeasurement LoudnessMeter::current() const {
    LoudnessMeasurement measurement;
    
    measurement.momentary_lufs = calculate_loudness(m_momentary_buffer);
    measurement.short_term_lufs = calculate_loudness(m_short_term_buffer);
    measurement.integrated_lufs = calculate_loudness(m_all_samples);
    
    measurement.true_peak_dbfs = m_true_peak_dbfs;
    
    return measurement;
}

} // namespace tachyon::audio
