#pragma once

#include <string>
#include <string_view>
#include <unordered_map>

namespace tachyon::output {

struct OutputPreset {
    std::string codec;           // "h264", "h265", "prores"
    std::string pixel_fmt;        // "yuv420p", "yuv444p10le"
    int         crf{23};
    std::string color_space;       // "bt709"
    std::string color_range;       // "limited"
    bool        faststart{true};
    int         width{0};          // 0 means use source
    int         height{0};        // 0 means use source
};

const OutputPreset* find_preset(std::string_view id);

// Initialize presets - call at startup
void register_default_presets();

} // namespace tachyon::output
