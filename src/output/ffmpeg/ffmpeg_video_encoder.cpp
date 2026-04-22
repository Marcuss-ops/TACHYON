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
        
        if (options.codec == "prores") {
            cmd += "-c:v prores_ks -profile:v 3 ";
        } else if (options.codec == "vp9") {
            cmd += "-c:v libvpx-vp9 ";
        } else {
            cmd += "-c:v libx264 -preset fast -crf 18 ";
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
        
        // Use a row-based buffer to reduce fwrite syscall overhead.
        // Each pixel is 4 bytes (RGBA).
        m_row_buffer.resize(width * 4);

        for (std::uint32_t y = 0; y < height; ++y) {
            for (std::uint32_t x = 0; x < width; ++x) {
                const renderer2d::Color color = frame.get_pixel(x, y);
                const std::size_t offset = x * 4;
                m_row_buffer[offset + 0] = static_cast<std::uint8_t>(std::clamp(renderer2d::detail::Linear_to_sRGB_f(color.r) * 255.0f, 0.0f, 255.0f));
                m_row_buffer[offset + 1] = static_cast<std::uint8_t>(std::clamp(renderer2d::detail::Linear_to_sRGB_f(color.g) * 255.0f, 0.0f, 255.0f));
                m_row_buffer[offset + 2] = static_cast<std::uint8_t>(std::clamp(renderer2d::detail::Linear_to_sRGB_f(color.b) * 255.0f, 0.0f, 255.0f));
                m_row_buffer[offset + 3] = static_cast<std::uint8_t>(std::clamp(color.a * 255.0f, 0.0f, 255.0f));
            }
            std::fwrite(m_row_buffer.data(), 1, m_row_buffer.size(), m_pipe);
        }
        
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
    std::vector<std::uint8_t> m_row_buffer;
};

} // namespace tachyon::output
