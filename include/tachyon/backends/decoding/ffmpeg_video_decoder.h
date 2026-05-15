#pragma once

#include "tachyon/core/media/decoding/video_decoder_interface.h"
#include <deque>
#include <mutex>

struct AVFormatContext;
struct AVCodecContext;
struct AVFrame;
struct AVPacket;
struct SwsContext;

namespace tachyon::backends::decoding {

class FfmpegVideoDecoder : public ::tachyon::core::media::IVideoDecoder {
public:
    FfmpegVideoDecoder() = default;
    ~FfmpegVideoDecoder() override { close(); }

    bool open(const std::filesystem::path& path) override;
    void close() override;

    std::optional<renderer2d::SurfaceRGBA> get_frame_at_time(double seconds) override;
    bool get_frame_into(double seconds, renderer2d::SurfaceRGBA& target) override;
    
    [[nodiscard]] double duration() const noexcept override { return m_duration_seconds; }
    [[nodiscard]] double frame_rate() const noexcept override { return m_frame_rate; }
    [[nodiscard]] int width() const noexcept override { return m_width; }
    [[nodiscard]] int height() const noexcept override { return m_height; }

private:
    struct CachedVideoFrame {
        double pts{0.0};
        renderer2d::SurfaceRGBA surface{1U, 1U};
    };

    std::optional<CachedVideoFrame> decode_frame_at_or_after(double seconds);
    std::optional<CachedVideoFrame> find_cached_frame(double seconds) const;
    void cache_frame(CachedVideoFrame frame);
    std::optional<CachedVideoFrame> convert_current_frame(double pts_seconds, AVFrame* frame);
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

} // namespace tachyon::backends::decoding
