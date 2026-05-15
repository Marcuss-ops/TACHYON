#include "tachyon/media/decoding/video_decoder.h"
#include "tachyon/media/decoding/svg_decoder.h"
#include "tachyon/backends/decoding/decoding_factory.h"
#include "tachyon/backends/decoding/ffmpeg_video_decoder.h"

namespace tachyon::media {

// VideoDecoder legacy implementation delegating to backends
bool VideoDecoder::open(const std::filesystem::path& path) {
    if (!m_impl) {
        m_impl = ::tachyon::backends::decoding::create_video_decoder();
    }
    return m_impl->open(path);
}

void VideoDecoder::close() {
    if (m_impl) {
        m_impl->close();
    }
}

std::optional<renderer2d::SurfaceRGBA> VideoDecoder::get_frame_at_time(double seconds) {
    return m_impl ? m_impl->get_frame_at_time(seconds) : std::nullopt;
}

bool VideoDecoder::get_frame_into(double seconds, renderer2d::SurfaceRGBA& target) {
    return m_impl ? m_impl->get_frame_into(seconds, target) : false;
}

double VideoDecoder::duration() const noexcept {
    return m_impl ? m_impl->duration() : 0.0;
}

double VideoDecoder::frame_rate() const noexcept {
    return m_impl ? m_impl->frame_rate() : 0.0;
}

int VideoDecoder::width() const noexcept {
    return m_impl ? m_impl->width() : 0;
}

int VideoDecoder::height() const noexcept {
    return m_impl ? m_impl->height() : 0;
}

// SVG Decoder redirect
bool parse_svg_string(const std::string& svg_content, ParsedSvg& out_result, DiagnosticBag& diagnostics) {
    return ::tachyon::core::media::parse_svg_string(svg_content, out_result, diagnostics);
}

} // namespace tachyon::media
