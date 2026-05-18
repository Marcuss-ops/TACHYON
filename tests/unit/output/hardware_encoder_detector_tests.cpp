#include "tachyon/output/hardware_encoder_detector.h"
#include <iostream>
#include <cstring>

namespace tachyon::output::test {

bool run_hardware_encoder_detector_tests() {
    using namespace tachyon::output;

    {
        std::string fake = R"(
 V....D h264_nvenc           NVIDIA NVENC H.264 encoder
 V....D hevc_nvenc           NVIDIA NVENC hevc encoder
 V....D libx264              libx264 H.264
)";
        auto caps = parse_ffmpeg_encoder_list(fake);
        auto chosen = choose_best_encoder(caps);

        if (chosen.backend != HardwareEncoderBackend::Nvenc) {
            std::cerr << "[EncoderDetector] expected NVENC\n";
            return false;
        }

        if (chosen.ffmpeg_codec != "h264_nvenc") {
            std::cerr << "[EncoderDetector] expected h264_nvenc\n";
            return false;
        }
    }

    {
        std::string fake = R"(
 V....D h264_vaapi           H.264/AVC VAAPI
 V....D libx264              libx264 H.264
)";
        auto caps = parse_ffmpeg_encoder_list(fake);
        auto chosen = choose_best_encoder(caps);

        if (chosen.backend != HardwareEncoderBackend::Vaapi) {
            std::cerr << "[EncoderDetector] expected VAAPI\n";
            return false;
        }

        bool has_format = false;
        bool has_hwupload = false;
        for (const auto& f : chosen.filters) {
            if (f == "format=nv12") has_format = true;
            if (f == "hwupload") has_hwupload = true;
        }

        if (!has_format || !has_hwupload) {
            std::cerr << "[EncoderDetector] VAAPI filters missing\n";
            return false;
        }
    }

    {
        std::string fake = R"(
 V....D libx264              libx264 H.264
)";
        auto caps = parse_ffmpeg_encoder_list(fake);
        auto chosen = choose_best_encoder(caps);

        if (chosen.backend != HardwareEncoderBackend::Software) {
            std::cerr << "[EncoderDetector] expected software fallback\n";
            return false;
        }

        if (chosen.ffmpeg_codec != "libx264") {
            std::cerr << "[EncoderDetector] expected libx264\n";
            return false;
        }
    }

    return true;
}

} // namespace tachyon::output::test
