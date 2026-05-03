#include "tachyon/presets/background/background_preset_registry.h"

namespace tachyon::presets {

std::optional<LayerSpec> build_background_preset(std::string_view id, int width, int height) {
    if (id != "blank_canvas") {
        return std::nullopt;
    }

    LayerSpec bg;
    bg.id = "bg_blank_canvas";
    bg.name = "Blank Canvas";
    bg.type = "solid";
    bg.kind = LayerType::Solid;
    bg.enabled = true;
    bg.visible = true;
    bg.start_time = 0.0;
    bg.in_point = 0.0;
    bg.out_point = 5.0;
    bg.width = width;
    bg.height = height;
    bg.opacity = 1.0;
    bg.fill_color.value = ColorSpec{16, 16, 16, 255};
    return bg;
}

std::vector<std::string> list_background_presets() {
    return {"blank_canvas"};
}

} // namespace tachyon::presets
