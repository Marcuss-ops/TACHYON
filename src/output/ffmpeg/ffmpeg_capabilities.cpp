#include "ffmpeg_capabilities.h"
#include "tachyon/core/platform/process.h"
#include <sstream>
#include <regex>

namespace tachyon::output {

FFmpegCapabilities detect_ffmpeg_capabilities() {
    core::platform::ProcessSpec spec;
    spec.executable = "ffmpeg";
    spec.args = {"-hide_banner", "-encoders"};
    
    auto result = core::platform::run_process(spec);
    if (!result.success) {
        return FFmpegCapabilities{};
    }

    auto caps = parse_ffmpeg_encoders(result.output);
    if (caps.has_nvenc) {
        // Run a real probe to check if NVENC actually works in this environment
        core::platform::ProcessSpec probe_spec;
        probe_spec.executable = "ffmpeg";
        probe_spec.args = {"-nostdin", "-f", "lavfi", "-i", "color=c=black:s=16x16", "-c:v", "h264_nvenc", "-t", "0.01", "-f", "null", "-"};
        auto probe_result = core::platform::run_process(probe_spec);
        if (!probe_result.success) {
            caps.has_nvenc = false;
        }
    }
    return caps;
}

FFmpegCapabilities parse_ffmpeg_encoders(const std::string& output) {
    FFmpegCapabilities caps;
    std::istringstream iss(output);
    std::string line;
    
    // Simpler regex: look for any sequence of 6 non-space chars followed by space and name
    std::regex encoder_regex(R"(^\s*([^\s]{6})\s+([a-z0-9_]+)\s+(.+)$)", std::regex::icase);
    
    while (std::getline(iss, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        
        std::smatch match;
        if (std::regex_search(line, match, encoder_regex)) {
            EncoderCapability cap;
            cap.name = match[2].str();
            cap.description = match[3].str();
            cap.backend = EncoderBackend::Software;

            const std::string& name = cap.name;
            if (name.find("nvenc") != std::string::npos) {
                cap.backend = EncoderBackend::Nvenc;
                caps.has_nvenc = true;
            } else if (name.find("vaapi") != std::string::npos) {
                cap.backend = EncoderBackend::Vaapi;
                caps.has_vaapi = true;
            } else if (name.find("videotoolbox") != std::string::npos) {
                cap.backend = EncoderBackend::VideoToolbox;
                caps.has_videotoolbox = true;
            } else if (name.find("amf") != std::string::npos) {
                cap.backend = EncoderBackend::Amf;
                caps.has_amf = true;
            }

            if (name.find("h264") != std::string::npos || name.find("x264") != std::string::npos) {
                cap.supports_h264 = true;
            }
            if (name.find("hevc") != std::string::npos || name.find("x265") != std::string::npos || name.find("h265") != std::string::npos) {
                cap.supports_hevc = true;
            }

            caps.encoders.push_back(std::move(cap));
        }
    }
    
    return caps;
}

std::string choose_ffmpeg_encoder(const std::string& requested, const FFmpegCapabilities& caps) {
    if (requested == "nvenc") return "h264_nvenc";
    if (requested == "vaapi") return "h264_vaapi";
    if (requested == "videotoolbox") return "h264_videotoolbox";
    if (requested == "amf") return "h264_amf";
    if (requested == "software") return "libx264";

    // Auto-detect if empty or "auto"
    if (requested.empty() || requested == "auto") {
        if (caps.has_nvenc) return "h264_nvenc";
        if (caps.has_videotoolbox) return "h264_videotoolbox";
        if (caps.has_amf) return "h264_amf";
        // VAAPI is often tricky without extra config, so we might prefer libx264 as default over VAAPI
        // unless explicitly requested.
    }

    return "libx264";
}

} // namespace tachyon::output
