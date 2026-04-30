#include "tachyon/media/decoding/video_decoder.h"

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

namespace tachyon::media {
namespace {

constexpr double kDefaultSeekToleranceSeconds = 1.0 / 30.0;

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

renderer2d::SurfaceRGBA surface_from_rgba_buffer(const std::vector<std::uint8_t>& buffer, int width, int height) {
    renderer2d::SurfaceRGBA surface(static_cast<std::uint32_t>(width), static_cast<std::uint32_t>(height));
    const std::size_t stride = static_cast<std::size_t>(width) * 4U;
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const std::size_t index = static_cast<std::size_t>(y) * stride + static_cast<std::size_t>(x) * 4U;
            surface.set_pixel(
                static_cast<std::uint32_t>(x),
                static_cast<std::uint32_t>(y),
                renderer2d::Color{
                    static_cast<float>(buffer[index + 0U]),
                    static_cast<float>(buffer[index + 1U]),
                    static_cast<float>(buffer[index + 2U]),
                    static_cast<float>(buffer[index + 3U])
                });
        }
    }
    return surface;
}

} // namespace

void VideoDecoder::close() {
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
    m_frame_cache.clear();
}

bool VideoDecoder::open(const std::filesystem::path& path) {
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

void VideoDecoder::cache_frame(CachedVideoFrame frame) {
    for (auto it = m_frame_cache.begin(); it != m_frame_cache.end(); ++it) {
        if (std::abs(it->pts - frame.pts) < 1e-6) {
            m_frame_cache.erase(it);
            break;
        }
    }

    m_frame_cache.push_front(std::move(frame));
    while (m_frame_cache.size() > kMaxCachedFrames) {
        m_frame_cache.pop_back();
    }
}

std::optional<VideoDecoder::CachedVideoFrame> VideoDecoder::find_cached_frame(double seconds) const {
    if (m_frame_cache.empty()) {
        return std::nullopt;
    }

    const double tolerance = m_frame_rate > 0.0 ? (0.5 / m_frame_rate) : kDefaultSeekToleranceSeconds;
    const CachedVideoFrame* best = nullptr;
    double best_delta = std::numeric_limits<double>::max();
    for (const auto& frame : m_frame_cache) {
        const double delta = std::abs(frame.pts - seconds);
        if (delta < best_delta) {
            best_delta = delta;
            best = &frame;
        }
    }

    if (best && best_delta <= tolerance) {
        return *best;
    }
    return std::nullopt;
}

std::optional<VideoDecoder::CachedVideoFrame> VideoDecoder::convert_current_frame(double pts_seconds, AVFrame* frame) {
#if !defined(TACHYON_HAS_FFMPEG)
    (void)pts_seconds;
    (void)frame;
    return std::nullopt;
#else
    if (!frame || m_width <= 0 || m_height <= 0) {
        return std::nullopt;
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
        return std::nullopt;
    }

    std::vector<std::uint8_t> buffer(static_cast<std::size_t>(m_width) * static_cast<std::size_t>(m_height) * 4U, 0U);
    std::uint8_t* dst_data[4] = {buffer.data(), nullptr, nullptr, nullptr};
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
        return std::nullopt;
    }

    CachedVideoFrame output;
    output.pts = pts_seconds;
    output.surface = surface_from_rgba_buffer(buffer, m_width, m_height);
    return output;
#endif
}

std::optional<VideoDecoder::CachedVideoFrame> VideoDecoder::decode_frame_at_or_after(double seconds) {
#if !defined(TACHYON_HAS_FFMPEG)
    (void)seconds;
    return std::nullopt;
#else
    if (!m_format_context || !m_codec_context || !m_packet || !m_frame || m_stream_index < 0) {
        return std::nullopt;
    }

    // Optimization: avoid seeking if we are within a reasonable forward window
    const double sequential_window = 2.0; // seconds
    bool needs_seek = true;
    if (m_last_pts >= 0.0 && seconds >= m_last_pts && seconds <= m_last_pts + sequential_window) {
        needs_seek = false;
    }

    if (needs_seek) {
        const int64_t seek_target = m_stream_time_base > 0.0
            ? static_cast<int64_t>(seconds / m_stream_time_base)
            : 0;
        if (av_seek_frame(m_format_context, m_stream_index, seek_target, AVSEEK_FLAG_BACKWARD) < 0) {
            return std::nullopt;
        }
        avcodec_flush_buffers(m_codec_context);
        m_last_pts = -1.0;
    }

    std::optional<CachedVideoFrame> previous;
    std::optional<CachedVideoFrame> best;
    double best_delta = std::numeric_limits<double>::max();

    int consecutive_failures = 0;
    while (av_read_frame(m_format_context, m_packet) >= 0) {
        if (m_packet->stream_index != m_stream_index) {
            av_packet_unref(m_packet);
            continue;
        }

        if (avcodec_send_packet(m_codec_context, m_packet) >= 0) {
            while (avcodec_receive_frame(m_codec_context, m_frame) >= 0) {
                const double pts_seconds = frame_timestamp_seconds(m_frame, m_stream_time_base);
                m_last_pts = pts_seconds;
                
                auto converted = convert_current_frame(pts_seconds, m_frame);
                av_frame_unref(m_frame);
                if (!converted.has_value()) {
                    continue;
                }

                cache_frame(*converted);

                const double delta = std::abs(converted->pts - seconds);
                if (delta < best_delta) {
                    best_delta = delta;
                    best = converted;
                }

                if (previous.has_value() && converted->pts >= seconds) {
                    const double previous_delta = std::abs(previous->pts - seconds);
                    best = previous_delta <= delta ? previous : converted;
                    av_packet_unref(m_packet);
                    return best;
                }

                previous = converted;

                if (converted->pts >= seconds && !best.has_value()) {
                    best = converted;
                }
            }
            consecutive_failures = 0;
        } else {
            consecutive_failures++;
            if (consecutive_failures > 10) break;
        }

        av_packet_unref(m_packet);
        if (best.has_value() && best->pts >= seconds) {
            break;
        }
    }

    if (!best.has_value()) {
        return previous;
    }
    return best;
#endif
}

std::optional<renderer2d::SurfaceRGBA> VideoDecoder::get_frame_at_time(double seconds) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (seconds < 0.0) {
        return std::nullopt;
    }

    if (auto cached = find_cached_frame(seconds); cached.has_value()) {
        return cached->surface;
    }

    if (auto decoded = decode_frame_at_or_after(seconds); decoded.has_value()) {
        return decoded->surface;
    }

    return std::nullopt;
}

std::future<std::optional<renderer2d::SurfaceRGBA>> VideoDecoder::request_frame_async(double seconds) {
    return std::async(std::launch::async, [this, seconds]() {
        return get_frame_at_time(seconds);
    });
}

} // namespace tachyon::media
