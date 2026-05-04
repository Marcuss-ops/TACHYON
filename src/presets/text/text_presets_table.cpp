#include "tachyon/presets/text/text_preset_registry.h"
#include "tachyon/presets/builders_common.h"

namespace tachyon::presets {

void TextPresetRegistry::load_builtins() {
    // 1. Classic Title
    register_preset({
        "classic_title", "Classic Title", "Large centered title with Inter Bold.",
        [](const std::string& content, const TextParams& p) {
            LayerSpec l = make_base_layer("text_title", "Title", "text", p);
            l.text_content = content;
            l.font_id = "inter_bold";
            l.font_size.value = 120.0;
            l.fill_color.value = ColorSpec{255, 255, 255, 255};
            l.alignment = "center";
            l.transform.position_x = p.w / 2.0;
            l.transform.position_y = p.h / 2.0;
            return l;
        }
    });

    // 2. Minimal Subtitle
    register_preset({
        "minimal_subtitle", "Minimal Subtitle", "Small clean subtitle at the bottom.",
        [](const std::string& content, const TextParams& p) {
            LayerSpec l = make_base_layer("text_subtitle", "Subtitle", "text", p);
            l.text_content = content;
            l.font_id = "inter_regular";
            l.font_size.value = 48.0;
            l.fill_color.value = ColorSpec{200, 200, 200, 255};
            l.alignment = "center";
            l.transform.position_x = p.w / 2.0;
            l.transform.position_y = p.h * 0.85;
            return l;
        }
    });

    // 3. Tech Label
    register_preset({
        "tech_label", "Tech Label", "Monospace tech-style label.",
        [](const std::string& content, const TextParams& p) {
            LayerSpec l = make_base_layer("text_tech", "Label", "text", p);
            l.text_content = content;
            l.font_id = "roboto_mono";
            l.font_size.value = 32.0;
            l.fill_color.value = ColorSpec{0, 255, 128, 255}; // Cyan-Green
            l.alignment = "left";
            return l;
        }
    });
}

} // namespace tachyon::presets
