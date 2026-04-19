#include "tachyon/audio/audio_analyzer.h"

#include <algorithm>
#include <cmath>
#include <complex>
#include <cstdint>
#include <fstream>
#include <numeric>
#include <vector>

#if __has_include(<libavformat/avformat.h>) && __has_include(<libavcodec/avcodec.h>) && __has_include(<libswresample/swresample.h>) && __has_include(<libavutil/opt.h>)
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4244 4267 4305 4310)
#endif
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/channel_layout.h>
#include <libavutil/opt.h>
#include <libswresample/swresample.h>
}
#ifdef _MSC_VER
#pragma warning(pop)
#endif
#define TACHYON_HAS_FFMPEG 1
#endif

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

std::vector<float> decode_audio_samples(const std::filesystem::path& path, double sample_rate) {
    std::vector<float> samples;

#if defined(TACHYON_HAS_FFMPEG)
    AVFormatContext* format_context = nullptr;
    if (avformat_open_input(&format_context, path.string().c_str(), nullptr, nullptr) != 0 || !format_context) {
        return samples;
    }

    if (avformat_find_stream_info(format_context, nullptr) < 0) {
        avformat_close_input(&format_context);
        return samples;
    }

    const int audio_stream_index = av_find_best_stream(format_context, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
    if (audio_stream_index < 0) {
        avformat_close_input(&format_context);
        return samples;
    }

    AVStream* stream = format_context->streams[audio_stream_index];
    const AVCodec* codec = avcodec_find_decoder(stream->codecpar->codec_id);
    if (!codec) {
        avformat_close_input(&format_context);
        return samples;
    }

    AVCodecContext* codec_context = avcodec_alloc_context3(codec);
    if (!codec_context) {
        avformat_close_input(&format_context);
        return samples;
    }

    if (avcodec_parameters_to_context(codec_context, stream->codecpar) < 0 || avcodec_open2(codec_context, codec, nullptr) < 0) {
        avcodec_free_context(&codec_context);
        avformat_close_input(&format_context);
        return samples;
    }

    AVChannelLayout in_layout;
    AVChannelLayout out_layout;
    av_channel_layout_default(&in_layout, codec_context->ch_layout.nb_channels > 0 ? codec_context->ch_layout.nb_channels : 1);
    av_channel_layout_default(&out_layout, 1);

    SwrContext* swr_context = nullptr;
    if (swr_alloc_set_opts2(
            &swr_context,
            &out_layout,
            AV_SAMPLE_FMT_FLT,
            static_cast<int>(sample_rate),
            &in_layout,
            codec_context->sample_fmt,
            codec_context->sample_rate,
            0,
            nullptr) < 0 || !swr_context || swr_init(swr_context) < 0) {
        av_channel_layout_uninit(&in_layout);
        av_channel_layout_uninit(&out_layout);
        swr_free(&swr_context);
        avcodec_free_context(&codec_context);
        avformat_close_input(&format_context);
        return samples;
    }
    av_channel_layout_uninit(&in_layout);
    av_channel_layout_uninit(&out_layout);

    AVPacket* packet = av_packet_alloc();
    AVFrame* frame = av_frame_alloc();
    if (!packet || !frame) {
        av_packet_free(&packet);
        av_frame_free(&frame);
        swr_free(&swr_context);
        avcodec_free_context(&codec_context);
        avformat_close_input(&format_context);
        return samples;
    }

    std::vector<float> converted(16384U, 0.0f);

    auto append_converted = [&](AVFrame* decoded_frame) {
        const uint8_t** in_data = const_cast<const uint8_t**>(decoded_frame->extended_data);
        int out_samples = swr_get_out_samples(swr_context, decoded_frame->nb_samples);
        if (out_samples <= 0) {
            return;
        }

        if (static_cast<std::size_t>(out_samples) > converted.size()) {
            converted.resize(static_cast<std::size_t>(out_samples));
        }

        uint8_t* out_data[] = {reinterpret_cast<uint8_t*>(converted.data())};
        const int converted_samples = swr_convert(
            swr_context,
            out_data,
            out_samples,
            in_data,
            decoded_frame->nb_samples);
        if (converted_samples > 0) {
            samples.insert(samples.end(), converted.data(), converted.data() + converted_samples);
        }
    };

    while (av_read_frame(format_context, packet) >= 0) {
        if (packet->stream_index != audio_stream_index) {
            av_packet_unref(packet);
            continue;
        }

        if (avcodec_send_packet(codec_context, packet) >= 0) {
            while (avcodec_receive_frame(codec_context, frame) >= 0) {
                append_converted(frame);
                av_frame_unref(frame);
            }
        }
        av_packet_unref(packet);
    }

    if (avcodec_send_packet(codec_context, nullptr) >= 0) {
        while (avcodec_receive_frame(codec_context, frame) >= 0) {
            append_converted(frame);
            av_frame_unref(frame);
        }
    }

    av_packet_free(&packet);
    av_frame_free(&frame);
    swr_free(&swr_context);
    avcodec_free_context(&codec_context);
    avformat_close_input(&format_context);
#else
    (void)path;
    (void)sample_rate;
#endif

    return samples;
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

bool AudioAnalyzer::load(const std::filesystem::path& path, double sample_rate) {
    m_samples.clear();
    m_sample_rate = sample_rate;
    m_duration_seconds = 0.0;

    if (sample_rate <= 0.0) {
        return false;
    }

    m_samples = decode_audio_samples(path, sample_rate);
    if (m_samples.empty()) {
        return false;
    }

    m_duration_seconds = static_cast<double>(m_samples.size()) / m_sample_rate;
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
