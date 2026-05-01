#include "tachyon/output/output_presets.h"
#include <algorithm>

namespace tachyon::output {

namespace {
std::unordered_map<std::string, OutputPreset> s_presets;

void init_default_presets() {
    if (!s_presets.empty()) return;

    // youtube_1080p - h264, yuv420p, crf18, bt709, limited
    s_presets["youtube_1080p"] = OutputPreset{
        .codec = "h264",
        .pixel_fmt = "yuv420p",
        .crf = 18,
        .color_space = "bt709",
        .color_range = "limited",
        .faststart = true
    };

    // youtube_shorts - h264, yuv420p, crf18, bt709, limited, 1080x1920
    s_presets["youtube_shorts"] = OutputPreset{
        .codec = "h264",
        .pixel_fmt = "yuv420p",
        .crf = 18,
        .color_space = "bt709",
        .color_range = "limited",
        .faststart = true,
        .width = 1080,
        .height = 1920
    };

    // youtube_4k - h265, yuv420p, crf20
    s_presets["youtube_4k"] = OutputPreset{
        .codec = "h265",
        .pixel_fmt = "yuv420p",
        .crf = 20,
        .color_space = "bt709",
        .color_range = "limited",
        .faststart = true
    };

    // tiktok - h264, yuv420p, crf18, 1080x1920
    s_presets["tiktok"] = OutputPreset{
        .codec = "h264",
        .pixel_fmt = "yuv420p",
        .crf = 18,
        .color_space = "bt709",
        .color_range = "limited",
        .faststart = true,
        .width = 1080,
        .height = 1920
    };

    // preview - h264, yuv420p, crf28, fast encode
    s_presets["preview"] = OutputPreset{
        .codec = "h264",
        .pixel_fmt = "yuv420p",
        .crf = 28,
        .color_space = "bt709",
        .color_range = "limited",
        .faststart = true
    };

    // png_sequence - png frame-by-frame
    s_presets["png_sequence"] = OutputPreset{
        .codec = "png",
        .pixel_fmt = "rgb24",
        .crf = 0,
        .color_space = "srgb",
        .color_range = "full",
        .faststart = false
    };

    // prores_4444 - prores, yuva444p10le, alpha
    s_presets["prores_4444"] = OutputPreset{
        .codec = "prores",
        .pixel_fmt = "yuva444p10le",
        .crf = 0,
        .color_space = "bt709",
        .color_range = "limited",
        .faststart = false
    };
}
} // namespace

const OutputPreset* find_preset(std::string_view id) {
    init_default_presets();
    auto it = s_presets.find(std::string(id));
    if (it != s_presets.end()) {
        return &it->second;
    }
    return nullptr;
}

void register_default_presets() {
    init_default_presets();
}

} // namespace tachyon::output
