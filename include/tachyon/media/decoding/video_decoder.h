#pragma once

#include "tachyon/renderer2d/core/framebuffer.h"

#include <deque>
#include <filesystem>
#include <optional>
#include <mutex>
#include <future>
#include <thread>
#include <condition_variable>
#include <atomic>

struct AVFormatContext;
struct AVCodecContext;
struct AVFrame;
struct AVPacket;
struct SwsContext;

namespace tachyon::media {

class VideoDecoder {
public:
    bool open(const std::filesystem::path& path);
    void close();

    std::optional<renderer2d::SurfaceRGBA> get_frame_at_time(double seconds);
    bool get_frame_into(double seconds, renderer2d::SurfaceRGBA& target);
    std::future<std::optional<renderer2d::SurfaceRGBA>> request_frame_async(double seconds);

    [[nodiscard]] double duration() const noexcept { return m_duration_seconds; }
    [[nodiscard]] double frame_rate() const noexcept { return m_frame_rate; }
    [[nodiscard]] int width() const noexcept { return m_width; }
    [[nodiscard]] int height() const noexcept { return m_height; }

private:
    struct CachedVideoFrame {
        double pts{0.0};
        renderer2d::SurfaceRGBA surface{1U, 1U};
    };

    std::optional<CachedVideoFrame> decode_frame_at_or_after(double seconds);
    std::optional<CachedVideoFrame> find_cached_frame(double seconds) const;
    void cache_frame(CachedVideoFrame frame);
    std::optional<CachedVideoFrame> convert_current_frame(double pts_seconds, AVFrame* frame);
    
    // Convert to target surface directly to avoid extra copies
    bool convert_to_surface(AVFrame* frame, renderer2d::SurfaceRGBA& target);

    std::filesystem::path m_path;
    double m_duration_seconds{0.0};
    double m_frame_rate{0.0};
    int m_width{0};
    int m_height{0};
    
    mutable std::mutex m_mutex;
    AVFormatContext* m_format_context{nullptr};
    AVCodecContext* m_codec_context{nullptr};
    AVFrame* m_frame{nullptr};
    AVPacket* m_packet{nullptr};
    SwsContext* m_sws_context{nullptr};
    int m_stream_index{-1};
    double m_stream_time_base{0.0};
    double m_last_pts{-1.0};

    static constexpr std::size_t kMaxCachedFrames = 8;
    std::deque<CachedVideoFrame> m_frame_cache;
};

} // namespace tachyon::media
