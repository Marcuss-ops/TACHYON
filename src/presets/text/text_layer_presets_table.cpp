#include "tachyon/presets/text/text_layer_preset_registry.h"
#include "tachyon/presets/builders_common.h"

namespace tachyon::presets {

void TextLayerPresetRegistry::load_builtins() {
    using namespace registry;

    auto get_text_layer_params = [](const ParameterBag& bag) {
        struct {
            std::string content;
            double w, h;
            double in, out;
        } res;
        res.content = bag.get_or<std::string>("content", "");
        res.w = bag.get_or<double>("width", 1920.0);
        res.h = bag.get_or<double>("height", 1080.0);
        res.in = bag.get_or<double>("in_point", 0.0);
        res.out = bag.get_or<double>("out_point", 10.0);
        return res;
    };

    // 1. Classic Title
    register_spec({
        "tachyon.text.layer.classic_title",
        {"tachyon.text.layer.classic_title", "Classic Title", "Large centered title with Inter Bold.", "text.layer", {"title", "classic"}},
        {},
        [get_text_layer_params](const ParameterBag& bag) {
            auto p = get_text_layer_params(bag);
            LayerSpec l;
            l.id = "text_title";
            l.name = "Title";
            l.type = LayerType::Text;
            l.type_string = "text";
            l.enabled = true;
            l.visible = true;
            l.in_point = p.in;
            l.out_point = p.out;
            l.width = static_cast<int>(p.w);
            l.height = static_cast<int>(p.h);
            l.text_content = p.content;
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
    register_spec({
        "tachyon.text.layer.minimal_subtitle",
        {"tachyon.text.layer.minimal_subtitle", "Minimal Subtitle", "Small clean subtitle at the bottom.", "text.layer", {"subtitle", "minimal"}},
        {},
        [get_text_layer_params](const ParameterBag& bag) {
            auto p = get_text_layer_params(bag);
            LayerSpec l;
            l.id = "text_subtitle";
            l.name = "Subtitle";
            l.type = LayerType::Text;
            l.type_string = "text";
            l.enabled = true;
            l.visible = true;
            l.in_point = p.in;
            l.out_point = p.out;
            l.width = static_cast<int>(p.w);
            l.height = static_cast<int>(p.h);
            l.text_content = p.content;
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
    register_spec({
        "tachyon.text.layer.tech_label",
        {"tachyon.text.layer.tech_label", "Tech Label", "Monospace tech-style label.", "text.layer", {"label", "tech", "mono"}},
        {},
        [get_text_layer_params](const ParameterBag& bag) {
            auto p = get_text_layer_params(bag);
            LayerSpec l;
            l.id = "text_tech";
            l.name = "Label";
            l.type = LayerType::Text;
            l.type_string = "text";
            l.enabled = true;
            l.visible = true;
            l.in_point = p.in;
            l.out_point = p.out;
            l.width = static_cast<int>(p.w);
            l.height = static_cast<int>(p.h);
            l.text_content = p.content;
            l.font_id = "roboto_mono";
            l.font_size.value = 32.0;
            l.fill_color.value = ColorSpec{0, 255, 128, 255}; // Cyan-Green
            l.alignment = "left";
            return l;
        }
    });
}

} // namespace tachyon::presets
