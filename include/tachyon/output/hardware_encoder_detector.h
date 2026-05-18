#pragma once

#include "tachyon/core/api.h"
#include <string>
#include <vector>

namespace tachyon::output {

enum class HardwareEncoderBackend {
    Software,
    Nvenc,
    Vaapi,
    Qsv,
    Amf,
    Unknown
};

struct TACHYON_API HardwareEncoderCapability {
    HardwareEncoderBackend backend{HardwareEncoderBackend::Unknown};
    std::string ffmpeg_codec;
    std::string pixel_format;
    std::vector<std::string> filters;
    bool available{false};
    std::string reason;
};

TACHYON_API std::string to_string(HardwareEncoderBackend backend);

TACHYON_API std::vector<HardwareEncoderCapability>
parse_ffmpeg_encoder_list(const std::string& text);

TACHYON_API HardwareEncoderCapability
choose_best_encoder(const std::vector<HardwareEncoderCapability>& capabilities);

TACHYON_API std::vector<HardwareEncoderCapability>
detect_hardware_encoders(const std::string& ffmpeg_binary = "ffmpeg");

} // namespace tachyon::output
