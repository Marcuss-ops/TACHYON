#include "tachyon/audio/audio_analyzer.h"
#include "tachyon/audio/audio_decoder.h"

#include <algorithm>
#include <cmath>
#include <complex>
#include <cstdint>
#include <numeric>
#include <vector>

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


AudioBands analyze_window(const std::vector<float>& samples, double sample_rate, std::size_t begin_index, std::size_t sample_count) {
    AudioBands bands;
    if (samples.empty() || sample_rate <= 0.0 || sample_count == 0U || begin_index >= samples.size()) {
        return bands;
    }

    const std::size_t actual_count = std::min(sample_count, samples.size() - begin_index);
    if (actual_count == 0U) {
        return bands;
    }

    const std::size_t fft_size = next_power_of_two(std::max<std::size_t>(256U, actual_count));
    std::vector<std::complex<float>> spectrum(fft_size, {0.0f, 0.0f});

    double rms_accumulator = 0.0;
    for (std::size_t index = 0; index < actual_count; ++index) {
        const float window = actual_count > 1U
            ? 0.5f - 0.5f * std::cos(2.0f * static_cast<float>(std::acos(-1.0)) * static_cast<float>(index) / static_cast<float>(actual_count - 1U))
            : 1.0f;
        const float value = samples[begin_index + index] * window;
        spectrum[index] = {value, 0.0f};
        rms_accumulator += static_cast<double>(value) * static_cast<double>(value);
    }

    fft_inplace(spectrum);

    const float total_energy = band_energy(spectrum, sample_rate, 20.0, 20000.0);
    if (total_energy > 0.0f) {
        bands.bass = std::clamp(band_energy(spectrum, sample_rate, 20.0, 250.0) / total_energy, 0.0f, 1.0f);
        bands.mid = std::clamp(band_energy(spectrum, sample_rate, 250.0, 4000.0) / total_energy, 0.0f, 1.0f);
        bands.high = std::clamp(band_energy(spectrum, sample_rate, 4000.0, 20000.0) / total_energy, 0.0f, 1.0f);
        bands.presence = std::clamp(band_energy(spectrum, sample_rate, 2000.0, 5000.0) / total_energy, 0.0f, 1.0f);
    }

    bands.rms = static_cast<float>(std::sqrt(rms_accumulator / static_cast<double>(actual_count)));
    return bands;
}

} // namespace

expressions::AudioAnalysisData AudioAnalyzer::to_analysis_data(const AudioBands& bands) {
    expressions::AudioAnalysisData data;
    data.bass = static_cast<double>(bands.bass);
    data.mid = static_cast<double>(bands.mid);
    data.treble = static_cast<double>(bands.high); // Map legacy high band to treble
    data.rms = static_cast<double>(bands.rms);
    data.beat = 0.0; // Beat detection not yet implemented
    return data;
}

bool AudioAnalyzer::load(const std::filesystem::path& path, double /*sample_rate*/) {
    m_samples.clear();
    m_sample_rate = 48000.0; // AudioDecoder outputs fixed 48kHz
    m_duration_seconds = 0.0;

    AudioDecoder decoder;
    if (!decoder.open(path)) {
        return false;
    }

    m_duration_seconds = decoder.duration();
    const std::vector<float> decoded = decoder.decode_range(0.0, m_duration_seconds);
    
    if (decoded.empty()) {
        return false;
    }

    // Downmix stereo (or multichannel) to mono for analysis
    // AudioDecoder now provides Stereo at 48kHz by default in my implementation
    // But we use the sample_rate passed to load here if it differs (though I should probably fix that)
    
    m_samples.reserve(decoded.size() / 2U);
    for (std::size_t i = 0; i + 1 < decoded.size(); i += 2) {
        m_samples.push_back((decoded[i] + decoded[i + 1]) * 0.5f);
    }

    return true;
}

AudioBands AudioAnalyzer::analyze_frame(double composition_time_seconds, double window_seconds) const {
    if (!loaded()) {
        return {};
    }

    const double clamped_time = std::clamp(composition_time_seconds, 0.0, m_duration_seconds);
    const double window = std::max(1.0 / m_sample_rate, window_seconds);
    const double half_window = window * 0.5;
    const double start_time = std::max(0.0, clamped_time - half_window);
    const std::size_t begin_index = static_cast<std::size_t>(std::floor(start_time * m_sample_rate));
    const std::size_t sample_count = static_cast<std::size_t>(std::ceil(window * m_sample_rate));
    return analyze_window(m_samples, m_sample_rate, begin_index, sample_count);
}

} // namespace tachyon::audio
