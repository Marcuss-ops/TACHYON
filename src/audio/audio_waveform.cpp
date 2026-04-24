#include "tachyon/audio/waveform.h"
#include <algorithm>
#include <cmath>
#include <complex>
#include <vector>
#include <numeric>

namespace tachyon::audio {

namespace {
std::size_t next_power_of_two(std::size_t value) {
    std::size_t power = 1;
    while (power < value) {
        power <<= 1U;
    }
    return power;
}

void fft_inplace(std::vector<std::complex<float>>& data) {
    const std::size_t n = data.size();
    if (n <= 1) {
        return;
    }

    std::size_t j = 0;
    for (std::size_t i = 1; i < n; ++i) {
        std::size_t bit = n >> 1U;
        while (j & bit) {
            j ^= bit;
            bit >>= 1U;
        }
        j ^= bit;
        if (i < j) {
            std::swap(data[i], data[j]);
        }
    }

    for (std::size_t len = 2; len <= n; len <<= 1U) {
        const float angle = -2.0f * static_cast<float>(std::acos(-1.0)) / static_cast<float>(len);
        const std::complex<float> wlen(std::cos(angle), std::sin(angle));
        for (std::size_t offset = 0; offset < n; offset += len) {
            std::complex<float> w{1.0f, 0.0f};
            for (std::size_t i = 0; i < len / 2U; ++i) {
                const std::complex<float> u = data[offset + i];
                const std::complex<float> v = data[offset + i + len / 2U] * w;
                data[offset + i] = u + v;
                data[offset + i + len / 2U] = u - v;
                w *= wlen;
            }
        }
    }
}

float band_energy(const std::vector<std::complex<float>>& spectrum, double sample_rate, double low_hz, double high_hz) {
    if (spectrum.empty() || sample_rate <= 0.0 || high_hz <= low_hz) {
        return 0.0f;
    }

    const double bin_hz = sample_rate / static_cast<double>(spectrum.size());
    float total = 0.0f;
    for (std::size_t bin = 0; bin < spectrum.size() / 2U; ++bin) {
        const double frequency = static_cast<double>(bin) * bin_hz;
        if (frequency < low_hz || frequency >= high_hz) {
            continue;
        }
        const float magnitude = std::norm(spectrum[bin]);
        total += magnitude;
    }
    return total;
}
} // namespace

class AudioReactivityEngine::Impl {
public:
    ReactivitySettings settings;
    BandLevels smoothed_levels;

    BandLevels process(const float* interleaved_samples, int num_samples, int channels, int sample_rate) {
        if (!interleaved_samples || num_samples <= 0 || channels <= 0 || sample_rate <= 0) {
            return BandLevels{};
        }

        // Convert interleaved multi-channel audio to mono
        std::vector<float> mono_samples;
        mono_samples.reserve(num_samples / channels);
        for (int i = 0; i < num_samples; i += channels) {
            float sum = 0.0f;
            for (int c = 0; c < channels; ++c) {
                sum += interleaved_samples[i + c];
            }
            mono_samples.push_back(sum / static_cast<float>(channels));
        }

        if (mono_samples.empty()) {
            return BandLevels{};
        }

        // Prepare FFT input
        const std::size_t fft_size = next_power_of_two(mono_samples.size());
        std::vector<std::complex<float>> spectrum(fft_size, {0.0f, 0.0f});
        for (std::size_t i = 0; i < mono_samples.size(); ++i) {
            spectrum[i] = {mono_samples[i], 0.0f};
        }
        fft_inplace(spectrum);

        // Calculate total energy (20Hz - 20kHz)
        const float total_energy = band_energy(spectrum, sample_rate, 20.0, 20000.0);
        BandLevels raw_levels;

        if (total_energy > 0.0f) {
            if (settings.enable_sub_bass) {
                raw_levels.sub_bass = std::clamp(
                    band_energy(spectrum, sample_rate, 20.0, 60.0) / total_energy,
                    0.0f, 1.0f);
            }
            if (settings.enable_bass) {
                raw_levels.bass = std::clamp(
                    band_energy(spectrum, sample_rate, 60.0, 250.0) / total_energy,
                    0.0f, 1.0f);
            }
            raw_levels.low_mid = std::clamp(
                band_energy(spectrum, sample_rate, 250.0, 500.0) / total_energy,
                0.0f, 1.0f);
            if (settings.enable_mid) {
                raw_levels.mid = std::clamp(
                    band_energy(spectrum, sample_rate, 500.0, 2000.0) / total_energy,
                    0.0f, 1.0f);
            }
            raw_levels.high_mid = std::clamp(
                band_energy(spectrum, sample_rate, 2000.0, 4000.0) / total_energy,
                0.0f, 1.0f);
            if (settings.enable_treble) {
                raw_levels.treble = std::clamp(
                    band_energy(spectrum, sample_rate, 4000.0, 20000.0) / total_energy,
                    0.0f, 1.0f);
            }
        }

        // Calculate overall RMS
        double rms_accum = 0.0;
        for (float s : mono_samples) {
            rms_accum += static_cast<double>(s) * static_cast<double>(s);
        }
        raw_levels.overall = static_cast<float>(std::sqrt(rms_accum / mono_samples.size()));

        // Apply temporal smoothing
        const float smooth = settings.smoothing_factor;
        BandLevels smoothed;
        smoothed.sub_bass = smoothed_levels.sub_bass * smooth + raw_levels.sub_bass * (1.0f - smooth);
        smoothed.bass = smoothed_levels.bass * smooth + raw_levels.bass * (1.0f - smooth);
        smoothed.low_mid = smoothed_levels.low_mid * smooth + raw_levels.low_mid * (1.0f - smooth);
        smoothed.mid = smoothed_levels.mid * smooth + raw_levels.mid * (1.0f - smooth);
        smoothed.high_mid = smoothed_levels.high_mid * smooth + raw_levels.high_mid * (1.0f - smooth);
        smoothed.treble = smoothed_levels.treble * smooth + raw_levels.treble * (1.0f - smooth);
        smoothed.overall = smoothed_levels.overall * smooth + raw_levels.overall * (1.0f - smooth);

        // Apply sensitivity multiplier
        smoothed.sub_bass *= settings.sensitivity;
        smoothed.bass *= settings.sensitivity;
        smoothed.low_mid *= settings.sensitivity;
        smoothed.mid *= settings.sensitivity;
        smoothed.high_mid *= settings.sensitivity;
        smoothed.treble *= settings.sensitivity;
        smoothed.overall *= settings.sensitivity;

        // Clamp final values to valid range
        smoothed.sub_bass = std::clamp(smoothed.sub_bass, 0.0f, 1.0f);
        smoothed.bass = std::clamp(smoothed.bass, 0.0f, 1.0f);
        smoothed.low_mid = std::clamp(smoothed.low_mid, 0.0f, 1.0f);
        smoothed.mid = std::clamp(smoothed.mid, 0.0f, 1.0f);
        smoothed.high_mid = std::clamp(smoothed.high_mid, 0.0f, 1.0f);
        smoothed.treble = std::clamp(smoothed.treble, 0.0f, 1.0f);
        smoothed.overall = std::clamp(smoothed.overall, 0.0f, 1.0f);

        smoothed_levels = smoothed;
        return smoothed;
    }
};

AudioReactivityEngine::AudioReactivityEngine() : impl_(std::make_unique<Impl>()) {}
AudioReactivityEngine::~AudioReactivityEngine() = default;

void AudioReactivityEngine::set_settings(const ReactivitySettings& settings) {
    impl_->settings = settings;
}

AudioReactivityEngine::ReactivitySettings AudioReactivityEngine::get_settings() const {
    return impl_->settings;
}

AudioReactivityEngine::BandLevels AudioReactivityEngine::process_audio_block(
    const float* interleaved_samples, int num_samples, int channels, int sample_rate) {
    return impl_->process(interleaved_samples, num_samples, channels, sample_rate);
}

AudioReactivityEngine::BandLevels AudioReactivityEngine::get_smoothed_levels() const {
    return impl_->smoothed_levels;
}

void AudioReactivityEngine::reset() {
    impl_->smoothed_levels = BandLevels{};
}

} // namespace tachyon::audio