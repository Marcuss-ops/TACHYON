#pragma once

#include "tachyon/core/media/media_interfaces.h"
#include <memory>

namespace tachyon::backends::ffmpeg {

class FFmpegEncoder : public core::media::IVideoEncoder {
public:
    FFmpegEncoder();
    ~FFmpegEncoder() override;

    core::MediaResult<void> open(const std::filesystem::path& output_path, int width, int height, double fps) override;
    core::MediaResult<void> write_frame(const renderer2d::SurfaceRGBA& surface) override;
    void close() override;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace tachyon::backends::ffmpeg
