#include "tachyon/audio/io/audio_decoder.h"

#include <algorithm>
#include <cmath>

#if __has_include(<libavformat/avformat.h>) && __has_include(<libavcodec/avcodec.h>) && __has_include(<libswresample/swresample.h>) && __has_include(<libavutil/channel_layout.h>)
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable: 4244 4267)
#endif
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/channel_layout.h>
#include <libavutil/opt.h>
#include <libswresample/swresample.h>
}
#if defined(_MSC_VER)
#pragma warning(pop)
#endif
#define TACHYON_HAS_FFMPEG 1
#endif

namespace tachyon::audio {

AudioDecoder::AudioDecoder() = default;

AudioDecoder::~AudioDecoder() {
    close();
}

void AudioDecoder::close() {
    std::lock_guard<std::mutex> lock(m_mutex);
#if defined(TACHYON_HAS_FFMPEG)
    if (m_swr_context) {
        swr_free(&m_swr_context);
    }
    if (m_packet) {
        av_packet_free(&m_packet);
    }
    if (m_frame) {
        av_frame_free(&m_frame);
    }
    if (m_codec_context) {
        avcodec_free_context(&m_codec_context);
    }
    if (m_format_context) {
        avformat_close_input(&m_format_context);
    }
#endif
    m_format_context = nullptr;
    m_codec_context = nullptr;
    m_frame = nullptr;
    m_packet = nullptr;
    m_swr_context = nullptr;
    m_stream_index = -1;
    m_path.clear();
    m_duration_seconds = 0.0;
}

bool AudioDecoder::open(const std::filesystem::path& path) {
    close();
    std::lock_guard<std::mutex> lock(m_mutex);
    m_path = path;

#if !defined(TACHYON_HAS_FFMPEG)
    fprintf(stderr, "[AudioDecoder] Error: FFmpeg not found during compilation. Cannot open %s\n", path.string().c_str());
    return false;
#else
    int ret = avformat_open_input(&m_format_context, path.string().c_str(), nullptr, nullptr);
    if (ret != 0) {
        char errbuf[256];
        av_strerror(ret, errbuf, sizeof(errbuf));
        fprintf(stderr, "[AudioDecoder] Error: Could not open input file %s: %s\n", path.string().c_str(), errbuf);
        return false;
    }

    if (avformat_find_stream_info(m_format_context, nullptr) < 0) {
        fprintf(stderr, "[AudioDecoder] Error: Could not find stream info for %s\n", path.string().c_str());
        return false;
    }

    m_stream_index = av_find_best_stream(m_format_context, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
    if (m_stream_index < 0) {
        fprintf(stderr, "[AudioDecoder] Error: Could not find audio stream in %s\n", path.string().c_str());
        return false;
    }

    AVStream* stream = m_format_context->streams[m_stream_index];
    const AVCodec* codec = avcodec_find_decoder(stream->codecpar->codec_id);
    if (!codec) {
        fprintf(stderr, "[AudioDecoder] Error: Decoder not found for codec ID %d in %s\n", stream->codecpar->codec_id, path.string().c_str());
        return false;
    }

    m_codec_context = avcodec_alloc_context3(codec);
    if (!m_codec_context) {
        fprintf(stderr, "[AudioDecoder] Error: Could not allocate codec context for %s\n", path.string().c_str());
        return false;
    }

    if (avcodec_parameters_to_context(m_codec_context, stream->codecpar) < 0) {
        fprintf(stderr, "[AudioDecoder] Error: Could not copy codec parameters for %s\n", path.string().c_str());
        return false;
    }

    if (avcodec_open2(m_codec_context, codec, nullptr) < 0) {
        fprintf(stderr, "[AudioDecoder] Error: Could not open codec for %s\n", path.string().c_str());
        return false;
    }

    m_channels = m_codec_context->ch_layout.nb_channels;
    m_sample_rate = m_codec_context->sample_rate;
    m_duration_seconds = static_cast<double>(m_format_context->duration) / static_cast<double>(AV_TIME_BASE);
    m_stream_time_base = av_q2d(stream->time_base);

    AVChannelLayout out_layout;
    av_channel_layout_default(&out_layout, kTargetChannels);

    if (swr_alloc_set_opts2(
            &m_swr_context,
            &out_layout,
            AV_SAMPLE_FMT_FLT,
            kTargetSampleRate,
            &m_codec_context->ch_layout,
            m_codec_context->sample_fmt,
            m_codec_context->sample_rate,
            0,
            nullptr) < 0 || !m_swr_context || swr_init(m_swr_context) < 0) {
        fprintf(stderr, "[AudioDecoder] Error: Could not initialize resampler for %s\n", path.string().c_str());
        return false;
    }

    m_packet = av_packet_alloc();
    m_frame = av_frame_alloc();
    if (!m_packet || !m_frame) {
        fprintf(stderr, "[AudioDecoder] Error: Could not allocate FFmpeg primitives for %s\n", path.string().c_str());
        return false;
    }
    return true;
#endif
}

std::vector<float> AudioDecoder::decode_range(double start_sec, double duration_sec) {
    (void)start_sec;
    (void)duration_sec;
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<float> samples;

#if !defined(TACHYON_HAS_FFMPEG)
    return samples;
#else
    if (!m_format_context || !m_codec_context || !m_swr_context) {
        return samples;
    }

    const int64_t seek_target = static_cast<int64_t>(start_sec / m_stream_time_base);
    if (av_seek_frame(m_format_context, m_stream_index, seek_target, AVSEEK_FLAG_BACKWARD) < 0) {
        return samples;
    }
    avcodec_flush_buffers(m_codec_context);

    const double end_sec = start_sec + duration_sec;
    std::vector<float> converted_buffer(16384U, 0.0f);

    while (av_read_frame(m_format_context, m_packet) >= 0) {
        if (m_packet->stream_index != m_stream_index) {
            av_packet_unref(m_packet);
            continue;
        }

        if (avcodec_send_packet(m_codec_context, m_packet) >= 0) {
            while (avcodec_receive_frame(m_codec_context, m_frame) >= 0) {
                const double pts_sec = static_cast<double>(m_frame->pts) * m_stream_time_base;
                
                // If we are significantly ahead, stop
                if (pts_sec > end_sec + 0.1) {
                    av_frame_unref(m_frame);
                    av_packet_unref(m_packet);
                    return samples;
                }

                int out_samples = swr_get_out_samples(m_swr_context, m_frame->nb_samples);
                if (out_samples > 0) {
                    if (static_cast<std::size_t>(out_samples * kTargetChannels) > converted_buffer.size()) {
                        converted_buffer.resize(static_cast<std::size_t>(out_samples * kTargetChannels));
                    }

                    uint8_t* out_data[] = {reinterpret_cast<uint8_t*>(converted_buffer.data())};
                    const int converted_count = swr_convert(
                        m_swr_context,
                        out_data,
                        out_samples,
                        const_cast<const uint8_t**>(m_frame->extended_data),
                        m_frame->nb_samples);

                    if (converted_count > 0) {
                        // Offset check: only add if it's within range
                        // This is a simple implementation, ideally we'd trim accurately
                        const double frame_end_sec = pts_sec + static_cast<double>(converted_count) / kTargetSampleRate;
                        if (frame_end_sec >= start_sec) {
                             samples.insert(samples.end(), converted_buffer.data(), converted_buffer.data() + converted_count * kTargetChannels);
                        }
                    }
                }
                av_frame_unref(m_frame);
            }
        }
        av_packet_unref(m_packet);
        
        if (samples.size() / kTargetChannels > static_cast<std::size_t>(duration_sec * kTargetSampleRate * 1.5)) {
            break;
        }
    }

    return samples;
#endif
}

} // namespace tachyon::audio

