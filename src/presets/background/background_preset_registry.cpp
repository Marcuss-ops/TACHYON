#include "tachyon/presets/background/background_preset_registry.h"
#include "tachyon/presets/background/procedural.h"
#include <string_view>
#include <vector>

namespace tachyon::presets {

namespace {

struct BackgroundPresetDef {
    std::string_view id;
    std::string_view name;
    void (*configure)(ProceduralSpec&);
};

const std::vector<BackgroundPresetDef>& get_preset_defs() {
    static const std::vector<BackgroundPresetDef> defs = {
        {"aurora_mesh", "Aurora Mesh", [](ProceduralSpec& p) {
            p.kind = "aura";
            p.seed = 42;
            p.color_a = AnimatedColorSpec(ColorSpec{5, 8, 20, 255});
            p.color_b = AnimatedColorSpec(ColorSpec{0, 178, 204, 255});
            p.color_c = AnimatedColorSpec(ColorSpec{192, 64, 255, 255});
            p.speed = AnimatedScalarSpec(0.18);
            p.frequency = AnimatedScalarSpec(1.8);
            p.amplitude = AnimatedScalarSpec(1.0);
            p.scale = AnimatedScalarSpec(1.0);
            p.grain_amount = AnimatedScalarSpec(0.03);
            p.contrast = AnimatedScalarSpec(1.08);
            p.softness = AnimatedScalarSpec(0.55);
            p.saturation = AnimatedScalarSpec(1.0);
        }},
        {"liquid_glass_blobs", "Liquid Glass Blobs", [](ProceduralSpec& p) {
            p.kind = "aura";
            p.seed = 1337;
            p.color_a = AnimatedColorSpec(ColorSpec{7, 10, 18, 255});
            p.color_b = AnimatedColorSpec(ColorSpec{6, 182, 212, 255});
            p.color_c = AnimatedColorSpec(ColorSpec{236, 72, 153, 255});
            p.speed = AnimatedScalarSpec(0.12);
            p.frequency = AnimatedScalarSpec(2.4);
            p.amplitude = AnimatedScalarSpec(1.0);
            p.scale = AnimatedScalarSpec(1.2);
            p.warp_strength = AnimatedScalarSpec(4.5);
            p.warp_frequency = AnimatedScalarSpec(1.8);
            p.warp_speed = AnimatedScalarSpec(0.8);
            p.grain_amount = AnimatedScalarSpec(0.02);
            p.contrast = AnimatedScalarSpec(1.0);
            p.softness = AnimatedScalarSpec(0.8);
            p.glow_intensity = AnimatedScalarSpec(0.45);
            p.saturation = AnimatedScalarSpec(1.05);
        }},
        {"dot_grid_fade", "Dot Grid Fade", [](ProceduralSpec& p) {
            p.kind = "grid";
            p.seed = 2468;
            p.shape = "circle";
            p.color_a = AnimatedColorSpec(ColorSpec{10, 10, 20, 255});
            p.color_b = AnimatedColorSpec(ColorSpec{94, 63, 183, 255});
            p.color_c = AnimatedColorSpec(ColorSpec{31, 41, 55, 255});
            p.speed = AnimatedScalarSpec(0.2);
            p.frequency = AnimatedScalarSpec(10.0);
            p.spacing = AnimatedScalarSpec(24.0);
            p.border_width = AnimatedScalarSpec(1.1);
            p.grain_amount = AnimatedScalarSpec(0.0);
            p.contrast = AnimatedScalarSpec(1.05);
            p.softness = AnimatedScalarSpec(0.2);
            p.glow_intensity = AnimatedScalarSpec(0.25);
        }},
        {"noise_grain_vignette", "Noise Grain Vignette", [](ProceduralSpec& p) {
            p.kind = "noise";
            p.seed = 8080;
            p.color_a = AnimatedColorSpec(ColorSpec{8, 9, 14, 255});
            p.color_b = AnimatedColorSpec(ColorSpec{28, 31, 44, 255});
            p.speed = AnimatedScalarSpec(0.08);
            p.frequency = AnimatedScalarSpec(14.0);
            p.amplitude = AnimatedScalarSpec(0.3);
            p.scale = AnimatedScalarSpec(1.8);
            p.grain_amount = AnimatedScalarSpec(0.08);
            p.contrast = AnimatedScalarSpec(0.95);
            p.gamma = AnimatedScalarSpec(1.08);
            p.saturation = AnimatedScalarSpec(0.0);
            p.softness = AnimatedScalarSpec(0.65);
        }},
        {"mesh_gradient", "Mesh Gradient", [](ProceduralSpec& p) {
            p.kind = "aura";
            p.seed = 2024;
            p.color_a = AnimatedColorSpec(ColorSpec{124, 58, 237, 255});
            p.color_b = AnimatedColorSpec(ColorSpec{6, 182, 212, 255});
            p.color_c = AnimatedColorSpec(ColorSpec{236, 72, 153, 255});
            p.speed = AnimatedScalarSpec(0.16);
            p.frequency = AnimatedScalarSpec(1.4);
            p.amplitude = AnimatedScalarSpec(1.0);
            p.scale = AnimatedScalarSpec(1.0);
            p.warp_strength = AnimatedScalarSpec(2.5);
            p.warp_frequency = AnimatedScalarSpec(1.2);
            p.warp_speed = AnimatedScalarSpec(0.5);
            p.grain_amount = AnimatedScalarSpec(0.01);
            p.contrast = AnimatedScalarSpec(1.0);
            p.softness = AnimatedScalarSpec(0.45);
            p.saturation = AnimatedScalarSpec(1.0);
        }},
        {"shape_grid_square", "Shape Grid Square", [](ProceduralSpec& p) {
            p.kind = "grid";
            p.shape = "square";
            p.seed = 1010;
            p.color_a = AnimatedColorSpec(ColorSpec{14, 11, 18, 255});
            p.color_b = AnimatedColorSpec(ColorSpec{38, 34, 51, 255});
            p.color_c = AnimatedColorSpec(ColorSpec{26, 22, 35, 255});
            p.spacing = AnimatedScalarSpec(46.0);
            p.border_width = AnimatedScalarSpec(1.0);
            p.mouse_influence = AnimatedScalarSpec(0.6);
            p.mouse_radius = AnimatedScalarSpec(1.0);
        }},
        {"dark_grain", "Dark Grain", [](ProceduralSpec& p) {
            p.kind = "noise";
            p.color_a = AnimatedColorSpec(ColorSpec{10, 10, 10, 255});
            p.grain_amount = AnimatedScalarSpec(0.015);
            p.vignette_intensity = AnimatedScalarSpec(1.4);
            p.contrast = AnimatedScalarSpec(1.0);
        }},
        {"white_paper", "White Paper", [](ProceduralSpec& p) {
            p.kind = "noise";
            p.color_a = AnimatedColorSpec(ColorSpec{250, 250, 250, 255});
            p.grain_amount = AnimatedScalarSpec(0.008);
            p.vignette_intensity = AnimatedScalarSpec(0.1);
            p.contrast = AnimatedScalarSpec(1.0);
        }},
        {"grid_dark", "Grid Dark", [](ProceduralSpec& p) {
            p.kind = "grid";
            p.color_a = AnimatedColorSpec(ColorSpec{5, 5, 5, 255});
            p.color_b = AnimatedColorSpec(ColorSpec{255, 255, 255, 18});
            p.spacing = AnimatedScalarSpec(12.0);
            p.border_width = AnimatedScalarSpec(1.0);
            p.softness = AnimatedScalarSpec(0.0);
        }},
        {"grid_white", "Grid White", [](ProceduralSpec& p) {
            p.kind = "grid";
            p.color_a = AnimatedColorSpec(ColorSpec{252, 252, 252, 255});
            p.color_b = AnimatedColorSpec(ColorSpec{0, 0, 0, 15});
            p.spacing = AnimatedScalarSpec(12.0);
            p.border_width = AnimatedScalarSpec(1.0);
            p.softness = AnimatedScalarSpec(0.0);
        }},
        {"soft_mesh", "Soft Mesh", [](ProceduralSpec& p) {
            p.kind = "aura";
            p.color_a = AnimatedColorSpec(ColorSpec{14, 14, 14, 255});
            p.color_b = AnimatedColorSpec(ColorSpec{23, 23, 23, 255});
            p.softness = AnimatedScalarSpec(0.8);
            p.glow_intensity = AnimatedScalarSpec(0.4);
            p.frequency = AnimatedScalarSpec(0.5);
        }},
        {"ripple_grid", "Ripple Grid", [](ProceduralSpec& p) {
            p.kind = "grid";
            p.color_a = AnimatedColorSpec(ColorSpec{106, 59, 255, 255});
            p.ripple_intensity = AnimatedScalarSpec(0.06);
            p.spacing = AnimatedScalarSpec(10.0);
            p.border_width = AnimatedScalarSpec(22.0);
            p.glow_intensity = AnimatedScalarSpec(0.35);
            p.mouse_influence = AnimatedScalarSpec(1.0);
            p.mouse_radius = AnimatedScalarSpec(1.0);
            p.vignette_intensity = AnimatedScalarSpec(2.8);
        }},
        {"apple_blobs", "Apple Blobs", [](ProceduralSpec& p) {
            p.kind = "aura";
            p.color_a = AnimatedColorSpec(ColorSpec{255, 126, 182, 180});
            p.color_b = AnimatedColorSpec(ColorSpec{139, 123, 255, 180});
            p.color_c = AnimatedColorSpec(ColorSpec{77, 184, 255, 180});
            p.speed = AnimatedScalarSpec(0.4);
            p.scale = AnimatedScalarSpec(1.5);
            p.softness = AnimatedScalarSpec(0.9);
        }},
        {"grid_bg", "Grid BG", [](ProceduralSpec& p) {
            p.kind = "grid";
            p.color_a = AnimatedColorSpec(ColorSpec{13, 13, 13, 255});
            p.color_b = AnimatedColorSpec(ColorSpec{30, 30, 30, 255});
            p.spacing = AnimatedScalarSpec(100.0);
            p.border_width = AnimatedScalarSpec(1.0);
            p.softness = AnimatedScalarSpec(0.0);
        }},
    };
    return defs;
}

LayerSpec make_procedural_background(std::string_view id, std::string_view name,
                                     int width, int height, double duration,
                                     void (*configure)(ProceduralSpec&)) {
    LayerSpec bg;
    bg.id = "bg_" + std::string(id);
    bg.name = std::string(name);
    bg.type = "procedural";
    bg.preset_id = std::string(id);
    bg.kind = LayerType::Procedural;
    bg.enabled = true;
    bg.visible = true;
    bg.start_time = 0.0;
    bg.in_point = 0.0;
    bg.out_point = duration;
    bg.width = width;
    bg.height = height;
    bg.opacity = 1.0;
    bg.procedural = ProceduralSpec{};
    configure(*bg.procedural);
    return bg;
}

} // namespace

std::optional<LayerSpec> build_background_preset(std::string_view id, int width,
                                                 int height) {
    if (id == "blank_canvas") {
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

    for (const auto& def : get_preset_defs()) {
        if (def.id == id) {
            return make_procedural_background(def.id, def.name, width, height, 8.0, def.configure);
        }
    }

    return std::nullopt;
}

std::vector<std::string> list_background_presets() {
    std::vector<std::string> ids;
    ids.reserve(get_preset_defs().size() + 1);
    ids.push_back("blank_canvas");
    for (const auto& def : get_preset_defs()) {
        ids.push_back(std::string(def.id));
    }
    return ids;
}

} // namespace tachyon::presets
