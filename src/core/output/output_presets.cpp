#include "tachyon/output/output_presets.h"
#include <algorithm>
#include <vector>

namespace tachyon::output {

namespace {
std::unordered_map<std::string, OutputPreset> s_presets;

void add_preset(std::string id, OutputPreset preset) {
    s_presets.emplace(std::move(id), std::move(preset));
}

void init_default_presets() {
    if (!s_presets.empty()) return;

    const OutputPreset youtube_1080p{
        .codec = "h264",
        .pixel_fmt = "yuv420p",
        .container = "mp4",
        .class_name = "video-export",
        .alpha_mode = "discarded",
        .rate_control_mode = "crf",
        .crf = 18,
        .color_space = "bt709",
        .color_range = "limited",
        .faststart = true
    };

    // youtube_1080p and aliases.
    add_preset("youtube_1080p", youtube_1080p);
    add_preset("youtube_1080p_30", youtube_1080p);
    add_preset("h264-mp4", youtube_1080p);

    const OutputPreset youtube_shorts{
        .codec = "h264",
        .pixel_fmt = "yuv420p",
        .container = "mp4",
        .class_name = "video-export",
        .alpha_mode = "discarded",
        .rate_control_mode = "crf",
        .crf = 18,
        .color_space = "bt709",
        .color_range = "limited",
        .faststart = true,
        .width = 1080,
        .height = 1920
    };

    // youtube_shorts and aliases.
    add_preset("youtube_shorts", youtube_shorts);
    add_preset("shorts", youtube_shorts);
    add_preset("tiktok", youtube_shorts);

    const OutputPreset youtube_4k{
        .codec = "h265",
        .pixel_fmt = "yuv420p",
        .container = "mp4",
        .class_name = "video-export",
        .alpha_mode = "discarded",
        .rate_control_mode = "crf",
        .crf = 20,
        .color_space = "bt709",
        .color_range = "limited",
        .faststart = true
    };

    add_preset("youtube_4k", youtube_4k);

    const OutputPreset preview{
        .codec = "h264",
        .pixel_fmt = "yuv420p",
        .container = "mp4",
        .class_name = "video-export",
        .alpha_mode = "discarded",
        .rate_control_mode = "crf",
        .crf = 18,
        .color_space = "bt709",
        .color_range = "limited",
        .faststart = true
    };

    add_preset("preview", preview);

    const OutputPreset png_sequence{
        .codec = "png",
        .pixel_fmt = "rgb24",
        .container = "png",
        .class_name = "image-sequence",
        .alpha_mode = "preserved",
        .rate_control_mode = "fixed",
        .crf = 0,
        .color_space = "srgb",
        .color_range = "full",
        .faststart = false
    };

    add_preset("png_sequence", png_sequence);
    add_preset("png-sequence", png_sequence);
    add_preset("png-seq", png_sequence);

    const OutputPreset prores_4444{
        .codec = "prores",
        .pixel_fmt = "yuva444p10le",
        .container = "mov",
        .class_name = "video-export",
        .alpha_mode = "preserved",
        .rate_control_mode = "fixed",
        .crf = 0,
        .color_space = "bt709",
        .color_range = "limited",
        .faststart = false
    };

    add_preset("prores_4444", prores_4444);

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

std::vector<std::string> list_presets() {
    init_default_presets();
    std::vector<std::string> ids;
    ids.reserve(s_presets.size());
    for (const auto& [id, _] : s_presets) {
        ids.push_back(id);
    }
    std::sort(ids.begin(), ids.end());
    return ids;
}

} // namespace tachyon::output
