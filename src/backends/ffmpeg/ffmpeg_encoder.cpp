#include "tachyon/backends/ffmpeg/ffmpeg_encoder.h"
#include <iostream>

namespace tachyon::backends::ffmpeg {

struct FFmpegEncoder::Impl {
    std::filesystem::path output_path;
    int width{0};
    int height{0};
    double fps{0.0};
};

FFmpegEncoder::~FFmpegEncoder() {
    close();
}

core::MediaResult<void> FFmpegEncoder::open(const std::filesystem::path& output_path, int width, int height, double fps) {
    m_impl = std::make_unique<Impl>();
    m_impl->output_path = output_path;
    m_impl->width = width;
    m_impl->height = height;
    m_impl->fps = fps;
    
    // In a real implementation, we would initialize FFmpeg contexts here
    std::cout << "[FFmpegEncoder] Opening " << output_path << " (" << width << "x" << height << " @ " << fps << " fps)\n";
    
    return core::MediaResult<void>::success();
}

core::MediaResult<void> FFmpegEncoder::write_frame(const renderer2d::SurfaceRGBA& surface) {
    if (!m_impl) return core::MediaResult<void>::failure(core::MediaError(core::MediaErrorCode::Init, "Encoder not opened"));
    
    // In a real implementation, we would encode the surface here
    return core::MediaResult<void>::success();
}

void FFmpegEncoder::close() {
    if (m_impl) {
        std::cout << "[FFmpegEncoder] Closing " << m_impl->output_path << "\n";
        m_impl.reset();
    }
}

} // namespace tachyon::backends::ffmpeg
