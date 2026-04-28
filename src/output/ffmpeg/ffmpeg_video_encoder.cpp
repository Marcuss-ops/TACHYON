#include "tachyon/output/video_encoder.h"
#include "tachyon/renderer2d/color/color_transfer.h"
#include <cstdio>
#include <string>
#include <vector>
#include <filesystem>

#if defined(_WIN32)
#define TACHYON_POPEN _popen
#define TACHYON_PCLOSE _pclose
#else
#define TACHYON_POPEN popen
#define TACHYON_PCLOSE pclose
#endif

namespace tachyon::output {

class FFmpegVideoEncoder final : public VideoEncoderBackend {
public:
    ~FFmpegVideoEncoder() override {
        finalize();
    }

    bool initialize(const std::string& output_path, const VideoEncoderOptions& options) override {
        m_last_error.clear();
        
        std::string cmd = "ffmpeg -y -f rawvideo -pix_fmt rgba -s " + 
                          std::to_string(options.width) + "x" + std::to_string(options.height) + 
                          " -r " + std::to_string(options.fps) + " -i - ";
        
        // Video codec and quality settings
        if (options.codec == "prores") {
            cmd += "-c:v prores_ks -profile:v 3 ";
        } else if (options.codec == "vp9") {
            cmd += "-c:v libvpx-vp9 ";
            if (options.bitrate_kbps > 0) {
                cmd += "-b:v " + std::to_string(options.bitrate_kbps) + "k ";
            } else {
                cmd += "-crf " + std::to_string(options.crf) + " ";
            }
        } else {
            // Default: H.264
            cmd += "-c:v libx264 ";
            cmd += "-preset " + options.preset + " ";
            if (options.bitrate_kbps > 0) {
                cmd += "-b:v " + std::to_string(options.bitrate_kbps) + "k ";
            } else {
                cmd += "-crf " + std::to_string(options.crf) + " ";
            }
        }
        
        // MP4 faststart for streaming
        if (options.faststart) {
            cmd += "-movflags +faststart ";
        }
        
        cmd += "\"" + output_path + "\"";
        
        m_pipe = TACHYON_POPEN(cmd.c_str(), "wb");
        if (!m_pipe) {
            m_last_error = "Failed to open ffmpeg pipe";
            return false;
        }
        
        m_options = options;
        return true;
    }

    bool push_frame(const renderer2d::Framebuffer& frame) override {
        if (!m_pipe) return false;

        const std::uint32_t width = frame.width();
        const std::uint32_t height = frame.height();
        const std::size_t frame_pixels = static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
        const std::size_t frame_bytes = frame_pixels * 4;
        
        // Pre-allocate full frame buffer to avoid per-frame allocation
        if (m_frame_buffer.size() < frame_bytes) {
            m_frame_buffer.resize(frame_bytes);
        }
        
        const auto& pixels = frame.pixels();
        
        // Direct vector access - pixels are stored as [r,g,b,a, r,g,b,a, ...] floats
        for (std::size_t i = 0; i < frame_pixels; ++i) {
            const std::size_t src_idx = i * 4;
            const std::size_t dst_idx = i * 4;
            m_frame_buffer[dst_idx + 0] = static_cast<std::uint8_t>(std::clamp(renderer2d::detail::Linear_to_sRGB_f(pixels[src_idx + 0]) * 255.0f, 0.0f, 255.0f));
            m_frame_buffer[dst_idx + 1] = static_cast<std::uint8_t>(std::clamp(renderer2d::detail::Linear_to_sRGB_f(pixels[src_idx + 1]) * 255.0f, 0.0f, 255.0f));
            m_frame_buffer[dst_idx + 2] = static_cast<std::uint8_t>(std::clamp(renderer2d::detail::Linear_to_sRGB_f(pixels[src_idx + 2]) * 255.0f, 0.0f, 255.0f));
            m_frame_buffer[dst_idx + 3] = static_cast<std::uint8_t>(std::clamp(pixels[src_idx + 3] * 255.0f, 0.0f, 255.0f));
        }
        
        // Single write for entire frame - much faster than per-row writes
        std::fwrite(m_frame_buffer.data(), 1, frame_bytes, m_pipe);
        
        return true;
    }

    bool finalize() override {
        if (m_pipe) {
            TACHYON_PCLOSE(m_pipe);
            m_pipe = nullptr;
        }
        return true;
    }

    const std::string& last_error() const override { return m_last_error; }

private:
    FILE* m_pipe{nullptr};
    VideoEncoderOptions m_options;
    std::string m_last_error;
    std::vector<std::uint8_t> m_frame_buffer;  // Pre-allocated full frame buffer
};

} // namespace tachyon::output
