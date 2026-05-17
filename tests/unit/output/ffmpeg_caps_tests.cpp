#include "test_utils.h"
#include "ffmpeg/ffmpeg_capabilities.h"
#include <iostream>

namespace tachyon::output::test {

bool test_detect_nvenc() {
    std::string fake_output = 
        " V....D h264_nvenc           NVIDIA NVENC H.264 encoder\n"
        " V....D hevc_nvenc           NVIDIA NVENC hevc encoder\n"
        " V....D libx264              libx264 H.264\n";
    
    auto caps = parse_ffmpeg_encoders(fake_output);
    if (!caps.has_nvenc) return false;
    if (caps.has_vaapi) return false;
    
    bool found_h264_nvenc = false;
    for (const auto& enc : caps.encoders) {
        if (enc.name == "h264_nvenc") {
            found_h264_nvenc = true;
            if (enc.backend != EncoderBackend::Nvenc) return false;
            if (!enc.supports_h264) return false;
        }
    }
    return found_h264_nvenc;
}

bool test_detect_vaapi() {
    std::string fake_output = 
        " V....D h264_vaapi           H.264/AVC VAAPI\n"
        " V....D hevc_vaapi           HEVC VAAPI\n";
    
    auto caps = parse_ffmpeg_encoders(fake_output);
    if (!caps.has_vaapi) return false;
    if (caps.has_nvenc) return false;
    return true;
}

bool test_software_fallback() {
    std::string fake_output = 
        " V....D libx264              libx264 H.264\n";
    
    auto caps = parse_ffmpeg_encoders(fake_output);
    if (caps.has_nvenc) return false;
    if (caps.has_vaapi) return false;
    return true;
}

bool test_choose_encoder() {
    FFmpegCapabilities caps;
    caps.has_nvenc = true;
    caps.has_vaapi = true;
    
    if (choose_ffmpeg_encoder("nvenc", caps) != "h264_nvenc") return false;
    if (choose_ffmpeg_encoder("vaapi", caps) != "h264_vaapi") return false;
    if (choose_ffmpeg_encoder("software", caps) != "libx264") return false;
    if (choose_ffmpeg_encoder("auto", caps) != "h264_nvenc") return false; // Nvenc preferred over Vaapi
    
    FFmpegCapabilities software_only;
    if (choose_ffmpeg_encoder("auto", software_only) != "libx264") return false;
    
    return true;
}

bool run_ffmpeg_caps_tests() {
    bool ok = true;
    if (!test_detect_nvenc()) { std::cerr << "test_detect_nvenc failed\n"; ok = false; }
    if (!test_detect_vaapi()) { std::cerr << "test_detect_vaapi failed\n"; ok = false; }
    if (!test_software_fallback()) { std::cerr << "test_software_fallback failed\n"; ok = false; }
    if (!test_choose_encoder()) { std::cerr << "test_choose_encoder failed\n"; ok = false; }
    return ok;
}

} // namespace tachyon::output::test
