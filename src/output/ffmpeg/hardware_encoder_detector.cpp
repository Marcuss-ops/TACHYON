#include "tachyon/output/hardware_encoder_detector.h"

#include <array>
#include <cstdio>
#include <sstream>

namespace tachyon::output {

std::string to_string(HardwareEncoderBackend backend) {
    switch (backend) {
        case HardwareEncoderBackend::Software: return "software";
        case HardwareEncoderBackend::Nvenc: return "nvenc";
        case HardwareEncoderBackend::Vaapi: return "vaapi";
        case HardwareEncoderBackend::Qsv: return "qsv";
        case HardwareEncoderBackend::Amf: return "amf";
        default: return "unknown";
    }
}

static bool contains(const std::string& text, const std::string& needle) {
    return text.find(needle) != std::string::npos;
}

std::vector<HardwareEncoderCapability>
parse_ffmpeg_encoder_list(const std::string& text) {
    std::vector<HardwareEncoderCapability> caps;

    auto add = [&](HardwareEncoderBackend backend,
                   const std::string& codec,
                   const std::string& pix_fmt,
                   std::vector<std::string> filters = {}) {
        if (contains(text, codec)) {
            HardwareEncoderCapability c;
            c.backend = backend;
            c.ffmpeg_codec = codec;
            c.pixel_format = pix_fmt;
            c.filters = std::move(filters);
            c.available = true;
            caps.push_back(std::move(c));
        }
    };

    add(HardwareEncoderBackend::Nvenc, "h264_nvenc", "yuv420p");
    add(HardwareEncoderBackend::Nvenc, "hevc_nvenc", "yuv420p");

    add(HardwareEncoderBackend::Qsv, "h264_qsv", "nv12");
    add(HardwareEncoderBackend::Qsv, "hevc_qsv", "nv12");

    add(HardwareEncoderBackend::Vaapi, "h264_vaapi", "nv12", {"format=nv12", "hwupload"});
    add(HardwareEncoderBackend::Vaapi, "hevc_vaapi", "nv12", {"format=nv12", "hwupload"});

    add(HardwareEncoderBackend::Amf, "h264_amf", "yuv420p");
    add(HardwareEncoderBackend::Amf, "hevc_amf", "yuv420p");

    add(HardwareEncoderBackend::Software, "libx264", "yuv420p");
    add(HardwareEncoderBackend::Software, "libx265", "yuv420p");

    if (caps.empty()) {
        HardwareEncoderCapability fallback;
        fallback.backend = HardwareEncoderBackend::Software;
        fallback.ffmpeg_codec = "libx264";
        fallback.pixel_format = "yuv420p";
        fallback.available = false;
        fallback.reason = "No known FFmpeg encoder found; defaulting to libx264.";
        caps.push_back(std::move(fallback));
    }

    return caps;
}

HardwareEncoderCapability choose_best_encoder(const std::vector<HardwareEncoderCapability>& caps) {
    auto find_backend = [&](HardwareEncoderBackend b) -> const HardwareEncoderCapability* {
        for (const auto& c : caps) {
            if (c.backend == b && c.available) return &c;
        }
        return nullptr;
    };

    if (auto* c = find_backend(HardwareEncoderBackend::Nvenc)) return *c;
    if (auto* c = find_backend(HardwareEncoderBackend::Qsv)) return *c;
    if (auto* c = find_backend(HardwareEncoderBackend::Vaapi)) return *c;
    if (auto* c = find_backend(HardwareEncoderBackend::Amf)) return *c;
    if (auto* c = find_backend(HardwareEncoderBackend::Software)) return *c;

    HardwareEncoderCapability fallback;
    fallback.backend = HardwareEncoderBackend::Software;
    fallback.ffmpeg_codec = "libx264";
    fallback.pixel_format = "yuv420p";
    fallback.available = false;
    fallback.reason = "No available encoder capability.";
    return fallback;
}

static std::string run_command_capture_stdout(const std::string& cmd) {
    std::array<char, 4096> buffer{};
    std::string result;

#if defined(_WIN32)
    FILE* pipe = _popen(cmd.c_str(), "r");
#else
    FILE* pipe = popen(cmd.c_str(), "r");
#endif

    if (!pipe) return {};

    while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe)) {
        result += buffer.data();
    }

#if defined(_WIN32)
    _pclose(pipe);
#else
    pclose(pipe);
#endif

    return result;
}

std::vector<HardwareEncoderCapability>
detect_hardware_encoders(const std::string& ffmpeg_binary) {
    std::string cmd = ffmpeg_binary + " -hide_banner -encoders 2>&1";
    std::string output = run_command_capture_stdout(cmd);
    return parse_ffmpeg_encoder_list(output);
}

} // namespace tachyon::output
