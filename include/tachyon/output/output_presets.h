#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>

namespace tachyon::output {

struct OutputPreset {
    std::string codec;           // "h264", "h265", "prores"
    std::string pixel_fmt;       // "yuv420p", "yuv444p10le"
    std::string container;       // "mp4", "png", "mov"
    std::string class_name;      // "video-export", "image-sequence"
    std::string alpha_mode;      // "discarded", "preserved"
    std::string rate_control_mode; // "crf", "fixed"
    int         crf{23};
    std::string color_space;       // "bt709"
    std::string color_range;       // "limited"
    bool        faststart{true};
    int         width{0};          // 0 means use source
    int         height{0};        // 0 means use source
};

const OutputPreset* find_preset(std::string_view id);
std::vector<std::string> list_presets();

// Initialize presets - call at startup
void register_default_presets();

} // namespace tachyon::output
