#include "tachyon/audio/io/audio_encoder.h"

#include <algorithm>
#include <cstring>
#include <iostream>

#if __has_include(<libavformat/avformat.h>) && __has_include(<libavcodec/avcodec.h>) && \
    __has_include(<libswresample/swresample.h>) && __has_include(<libavutil/channel_layout.h>)
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

#if defined(TACHYON_HAS_FFMPEG)
static const AVCodec* find_audio_encoder(const std::string& codec_name) {
    if (codec_name == "aac") return avcodec_find_encoder(AV_CODEC_ID_AAC);
    if (codec_name == "mp3") return avcodec_find_encoder(AV_CODEC_ID_MP3);
    if (codec_name == "flac") return avcodec_find_encoder(AV_CODEC_ID_FLAC);
    return avcodec_find_encoder_by_name(codec_name.c_str());
}
#endif

AudioEncoder::AudioEncoder() = default;

AudioEncoder::~AudioEncoder() {
    close();
}

void AudioEncoder::close() {
#if defined(TACHYON_HAS_FFMPEG)
    if (m_swr_context) {
        swr_free(&m_swr_context);
    }
    if (m_frame) {
        av_frame_free(&m_frame);
    }
    if (m_packet) {
        av_packet_free(&m_packet);
    }
    if (m_codec_context) {
        avcodec_free_context(&m_codec_context);
    }
    if (m_format_context) {
        avio_closep(&m_format_context->pb);
        avformat_free_context(m_format_context);
    }
#endif
    m_format_context = nullptr;
    m_codec_context = nullptr;
    m_stream = nullptr;
    m_frame = nullptr;
    m_packet = nullptr;
    m_swr_context = nullptr;
    m_pts = 0;
    m_opened = false;
}

bool AudioEncoder::open(const std::filesystem::path& output_path, const AudioExportConfig& config) {
    close();
    m_output_path = output_path;
    m_config = config;
    m_sample_rate = config.sample_rate;
    m_channels = config.channels;

#if !defined(TACHYON_HAS_FFMPEG)
    fprintf(stderr, "[AudioEncoder] Error: FFmpeg not found during compilation. Cannot open %s\n", output_path.string().c_str());
    return false;
#else
    const AVCodec* codec = find_audio_encoder(config.codec);
    if (!codec) {
        fprintf(stderr, "[AudioEncoder] Error: Could not find encoder for %s\n", config.codec.c_str());
        return false;
    }

    m_format_context = avformat_alloc_context();
    if (!m_format_context) {
        fprintf(stderr, "[AudioEncoder] Error: Could not allocate format context for %s\n", output_path.string().c_str());
        return false;
    }

    const char* format_name = nullptr;
    if (config.codec == "aac") format_name = "adts";
    else if (config.codec == "mp3") format_name = "mp3";
    else if (config.codec == "flac") format_name = "flac";

    AVIOContext* io = nullptr;
    int ret = avio_open(&io, output_path.string().c_str(), AVIO_FLAG_WRITE);
    if (ret < 0) {
        char errbuf[256];
        av_strerror(ret, errbuf, sizeof(errbuf));
        fprintf(stderr, "[AudioEncoder] Error: Could not open output file %s: %s\n", output_path.string().c_str(), errbuf);
        close();
        return false;
    }
    m_format_context->pb = io;

    if (format_name) {
        const AVOutputFormat* oformat = av_guess_format(format_name, nullptr, nullptr);
        if (oformat) m_format_context->oformat = const_cast<AVOutputFormat*>(oformat);
    }

    m_codec_context = avcodec_alloc_context3(codec);
    if (!m_codec_context) {
        fprintf(stderr, "[AudioEncoder] Error: Could not allocate codec context for %s\n", output_path.string().c_str());
        close();
        return false;
    }

    m_codec_context->sample_rate = config.sample_rate;
    m_codec_context->ch_layout = AV_CHANNEL_LAYOUT_STEREO;
    if (config.channels == 1) m_codec_context->ch_layout = AV_CHANNEL_LAYOUT_MONO;
    m_codec_context->bit_rate = config.bitrate_kbps * 1000;
    m_codec_context->sample_fmt = AV_SAMPLE_FMT_FLTP;
    if (config.codec == "mp3") m_codec_context->sample_fmt = AV_SAMPLE_FMT_S16P;
    else if (config.codec == "flac") m_codec_context->sample_fmt = AV_SAMPLE_FMT_S32;

    if (avcodec_open2(m_codec_context, codec, nullptr) < 0) {
        fprintf(stderr, "[AudioEncoder] Error: Could not open codec %s for %s\n", config.codec.c_str(), output_path.string().c_str());
        close();
        return false;
    }

    m_stream = avformat_new_stream(m_format_context, nullptr);
    if (!m_stream) {
        fprintf(stderr, "[AudioEncoder] Error: Could not create new stream for %s\n", output_path.string().c_str());
        close();
        return false;
    }
    m_stream->codecpar->codec_type = AVMEDIA_TYPE_AUDIO;
    m_stream->codecpar->codec_id = codec->id;
    m_stream->codecpar->sample_rate = config.sample_rate;
    m_stream->codecpar->ch_layout.nb_channels = config.channels;
    av_channel_layout_copy(&m_stream->codecpar->ch_layout, &m_codec_context->ch_layout);

    if (!(m_codec_context->codec->capabilities & AV_CODEC_CAP_VARIABLE_FRAME_SIZE)) {
        m_frame_size = m_codec_context->frame_size;
    }

    if (avformat_write_header(m_format_context, nullptr) < 0) {
        fprintf(stderr, "[AudioEncoder] Error: Could not write header for %s\n", output_path.string().c_str());
        close();
        return false;
    }

    m_frame = av_frame_alloc();
    if (!m_frame) {
        close();
        return false;
    }
    m_frame->sample_rate = config.sample_rate;
    m_frame->format = m_codec_context->sample_fmt;
    av_channel_layout_copy(&m_frame->ch_layout, &m_codec_context->ch_layout);

    m_packet = av_packet_alloc();
    if (!m_packet) {
        close();
        return false;
    }

    m_opened = true;
    return true;
#endif
}

bool AudioEncoder::encode_interleaved_float(const std::vector<float>& interleaved_samples) {
    (void)interleaved_samples;
#if !defined(TACHYON_HAS_FFMPEG)
    return false;
#else
    if (!m_opened || interleaved_samples.empty()) return false;

    int samples_per_channel = static_cast<int>(interleaved_samples.size()) / m_channels;
    if (samples_per_channel == 0) return false;

    m_frame->nb_samples = samples_per_channel;
    if (av_frame_get_buffer(m_frame, 0) < 0) return false;

    if (m_codec_context->sample_fmt == AV_SAMPLE_FMT_FLTP) {
        for (int ch = 0; ch < m_channels; ++ch) {
            float* dst = reinterpret_cast<float*>(m_frame->data[ch]);
            for (int i = 0; i < samples_per_channel; ++i) {
                dst[i] = interleaved_samples[i * m_channels + ch];
            }
        }
    } else {
        av_frame_make_writable(m_frame);
        if (m_channels == 2) {
            for (int i = 0; i < samples_per_channel; ++i) {
                if (m_codec_context->sample_fmt == AV_SAMPLE_FMT_S16P) {
                    reinterpret_cast<int16_t*>(m_frame->data[0])[i] = static_cast<int16_t>(interleaved_samples[i * 2] * 32767.0f);
                    reinterpret_cast<int16_t*>(m_frame->data[1])[i] = static_cast<int16_t>(interleaved_samples[i * 2 + 1] * 32767.0f);
                } else {
                    reinterpret_cast<int32_t*>(m_frame->data[0])[i] = static_cast<int32_t>(interleaved_samples[i * 2] * 2147483647.0f);
                    reinterpret_cast<int32_t*>(m_frame->data[1])[i] = static_cast<int32_t>(interleaved_samples[i * 2 + 1] * 2147483647.0f);
                }
            }
        }
    }

    m_frame->pts = m_pts;
    m_pts += samples_per_channel;

    int ret = avcodec_send_frame(m_codec_context, m_frame);
    av_frame_unref(m_frame);
    if (ret < 0) return false;

    while (ret >= 0) {
        ret = avcodec_receive_packet(m_codec_context, m_packet);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) break;
        if (ret < 0) return false;
        av_packet_rescale_ts(m_packet, m_codec_context->time_base, m_stream->time_base);
        m_packet->stream_index = m_stream->index;
        if (av_interleaved_write_frame(m_format_context, m_packet) < 0) return false;
        av_packet_unref(m_packet);
    }
    return true;
#endif
}

bool AudioEncoder::flush() {
#if !defined(TACHYON_HAS_FFMPEG)
    return false;
#else
    if (!m_opened) return false;

    int ret = avcodec_send_frame(m_codec_context, nullptr);
    while (ret >= 0) {
        ret = avcodec_receive_packet(m_codec_context, m_packet);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) break;
        if (ret < 0) return false;
        av_packet_rescale_ts(m_packet, m_codec_context->time_base, m_stream->time_base);
        m_packet->stream_index = m_stream->index;
        if (av_interleaved_write_frame(m_format_context, m_packet) < 0) return false;
        av_packet_unref(m_packet);
    }

    if (av_write_trailer(m_format_context) < 0) return false;
    m_opened = false;
    return true;
#endif
}

} // namespace tachyon::audio

