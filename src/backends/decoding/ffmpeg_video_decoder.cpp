#include "tachyon/backends/decoding/ffmpeg_video_decoder.h"
#include "tachyon/core/simd/conversion.h"
#include "tachyon/core/profiling.h"
#include <iostream>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <vector>

#if __has_include(<libavcodec/avcodec.h>) && __has_include(<libavformat/avformat.h>) && __has_include(<libswscale/swscale.h>) && __has_include(<libavutil/imgutils.h>)
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4244 4267 4305 4310)
#endif
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}
#ifdef _MSC_VER
#pragma warning(pop)
#endif
#define TACHYON_HAS_FFMPEG 1
#endif

namespace tachyon::backends::decoding {

namespace {

double frame_timestamp_seconds(const AVFrame* frame, double stream_time_base) {
#if !defined(TACHYON_HAS_FFMPEG)
    (void)frame;
    (void)stream_time_base;
    return 0.0;
#else
    if (!frame) {
        return 0.0;
    }

    const int64_t pts = frame->best_effort_timestamp != AV_NOPTS_VALUE
        ? frame->best_effort_timestamp
        : frame->pts;
    if (pts == AV_NOPTS_VALUE || stream_time_base <= 0.0) {
        return 0.0;
    }
    return static_cast<double>(pts) * stream_time_base;
#endif
}

} // namespace

void FfmpegVideoDecoder::close() {
    std::lock_guard<std::mutex> lock(m_mutex);
#if defined(TACHYON_HAS_FFMPEG)
    if (m_sws_context) {
        sws_freeContext(m_sws_context);
        m_sws_context = nullptr;
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
    m_stream_index = -1;
    m_stream_time_base = 0.0;
#endif
    m_path.clear();
    m_duration_seconds = 0.0;
    m_frame_rate = 0.0;
    m_width = 0;
    m_height = 0;
}

bool FfmpegVideoDecoder::open(const std::filesystem::path& path) {
    close();
    m_path = path;

#if !defined(TACHYON_HAS_FFMPEG)
    (void)path;
    return false;
#else
    if (path.empty()) {
        return false;
    }

    if (avformat_open_input(&m_format_context, path.string().c_str(), nullptr, nullptr) != 0 || !m_format_context) {
        close();
        return false;
    }

    if (avformat_find_stream_info(m_format_context, nullptr) < 0) {
        close();
        return false;
    }

    m_stream_index = av_find_best_stream(m_format_context, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    if (m_stream_index < 0) {
        close();
        return false;
    }

    AVStream* stream = m_format_context->streams[m_stream_index];
    const AVCodec* codec = avcodec_find_decoder(stream->codecpar->codec_id);
    if (!codec) {
        close();
        return false;
    }

    m_codec_context = avcodec_alloc_context3(codec);
    if (!m_codec_context) {
        close();
        return false;
    }

    if (avcodec_parameters_to_context(m_codec_context, stream->codecpar) < 0 || avcodec_open2(m_codec_context, codec, nullptr) < 0) {
        close();
        return false;
    }

    m_width = m_codec_context->width;
    m_height = m_codec_context->height;
    const AVRational rate = av_guess_frame_rate(m_format_context, stream, nullptr);
    const AVRational fallback_rate = stream->avg_frame_rate.num > 0 && stream->avg_frame_rate.den > 0
        ? stream->avg_frame_rate
        : stream->r_frame_rate;
    const AVRational chosen_rate = rate.num > 0 && rate.den > 0 ? rate : fallback_rate;
    m_frame_rate = chosen_rate.num > 0 && chosen_rate.den > 0 ? av_q2d(chosen_rate) : 0.0;

    if (m_format_context->duration > 0) {
        m_duration_seconds = static_cast<double>(m_format_context->duration) / static_cast<double>(AV_TIME_BASE);
    } else if (stream->duration > 0) {
        m_duration_seconds = static_cast<double>(stream->duration) * av_q2d(stream->time_base);
    }
    m_stream_time_base = av_q2d(stream->time_base);

    m_packet = av_packet_alloc();
    m_frame = av_frame_alloc();
    if (!m_packet || !m_frame) {
        close();
        return false;
    }

    m_last_pts = -1.0;
    return true;
#endif
}

bool FfmpegVideoDecoder::convert_to_surface(AVFrame* frame, renderer2d::SurfaceRGBA& target) {
#if !defined(TACHYON_HAS_FFMPEG)
    (void)frame; (void)target;
    return false;
#else
    if (!frame || m_width <= 0 || m_height <= 0) {
        return false;
    }

    m_sws_context = sws_getCachedContext(
        m_sws_context,
        m_width,
        m_height,
        static_cast<AVPixelFormat>(frame->format),
        m_width,
        m_height,
        AV_PIX_FMT_RGBA,
        SWS_BILINEAR,
        nullptr,
        nullptr,
        nullptr);

    if (!m_sws_context) {
        return false;
    }

    static thread_local std::vector<std::uint8_t> scratch_buffer;
    const std::size_t required_size = static_cast<std::size_t>(m_width) * static_cast<std::size_t>(m_height) * 4U;
    if (scratch_buffer.size() < required_size) {
        scratch_buffer.resize(required_size);
    }

    std::uint8_t* dst_data[4] = {scratch_buffer.data(), nullptr, nullptr, nullptr};
    int dst_linesize[4] = {m_width * 4, 0, 0, 0};
    
    const int scaled = sws_scale(
        m_sws_context,
        frame->data,
        frame->linesize,
        0,
        m_height,
        dst_data,
        dst_linesize);

    if (scaled <= 0) {
        return false;
    }
    
    if (target.width() != static_cast<uint32_t>(m_width) || target.height() != static_cast<uint32_t>(m_height)) {
        target.reset(static_cast<uint32_t>(m_width), static_cast<uint32_t>(m_height));
    }

    float* dst_pixels = target.mutable_pixels().data();
    const std::uint8_t* src_bytes = scratch_buffer.data();
    const std::size_t pixel_count = static_cast<std::size_t>(m_width) * m_height;

    // Fast conversion using SIMD
    ::tachyon::core::simd::rgba8_to_float32(dst_pixels, src_bytes, pixel_count);

    return true;
#endif
}

std::optional<renderer2d::SurfaceRGBA> FfmpegVideoDecoder::get_frame_at_time(double seconds) {
    renderer2d::SurfaceRGBA surface(1, 1);
    if (get_frame_into(seconds, surface)) {
        return surface;
    }
    return std::nullopt;
}

bool FfmpegVideoDecoder::get_frame_into(double seconds, renderer2d::SurfaceRGBA& target) {
    TACHYON_ZONE_F;
#if !defined(TACHYON_HAS_FFMPEG)
    (void)seconds; (void)target;
    return false;
#else
    std::lock_guard<std::mutex> lock(m_mutex);
    if (seconds < 0.0 || !m_format_context) {
        return false;
    }

    const double sequential_window = 2.0; 
    bool needs_seek = true;
    if (m_last_pts >= 0.0 && seconds >= m_last_pts && seconds <= m_last_pts + sequential_window) {
        needs_seek = false;
    }

    if (needs_seek) {
        const int64_t seek_target = m_stream_time_base > 0.0
            ? static_cast<int64_t>(seconds / m_stream_time_base)
            : 0;
        if (av_seek_frame(m_format_context, m_stream_index, seek_target, AVSEEK_FLAG_BACKWARD) < 0) {
            return false;
        }
        avcodec_flush_buffers(m_codec_context);
        m_last_pts = -1.0;
    }

    while (av_read_frame(m_format_context, m_packet) >= 0) {
        if (m_packet->stream_index != m_stream_index) {
            av_packet_unref(m_packet);
            continue;
        }

        if (avcodec_send_packet(m_codec_context, m_packet) >= 0) {
            while (avcodec_receive_frame(m_codec_context, m_frame) >= 0) {
                const double pts_seconds = frame_timestamp_seconds(m_frame, m_stream_time_base);
                m_last_pts = pts_seconds;
                
                if (pts_seconds >= seconds - 0.01) {
                    bool success = convert_to_surface(m_frame, target);
                    av_frame_unref(m_frame);
                    av_packet_unref(m_packet);
                    return success;
                }
                av_frame_unref(m_frame);
            }
        }
        av_packet_unref(m_packet);
    }

    return false;
#endif
}

} // namespace tachyon::backends::decoding
