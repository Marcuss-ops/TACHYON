#include "tachyon/presets/background/background_preset_registry.h"
#include "tachyon/presets/background/procedural.h"
#include "tachyon/presets/background/background_builders.h"
#include "tachyon/presets/builders_common.h"
#include <string_view>
#include <vector>

namespace tachyon::presets {

namespace {

struct BackgroundPresetDef {
    std::string id;
    std::string name;
    BackgroundParams params;
};

const std::vector<BackgroundPresetDef>& get_preset_defs() {
    static const std::vector<BackgroundPresetDef> defs = {
        {"aurora_mesh", "Aurora Mesh", {
            .kind = "aura", .palette = "neon_night", .seed = 42, .speed = 0.18f,
            .frequency = 1.8f, .amplitude = 1.0f, .scale = 1.0f, .contrast = 1.08f, .softness = 0.55f, .grain_amount = 0.03f,
            .color_a = ColorSpec{5, 8, 20, 255}, .color_b = ColorSpec{0, 178, 204, 255}, .color_c = ColorSpec{192, 64, 255, 255}
        }},
        {"liquid_glass_blobs", "Liquid Glass Blobs", {
            .kind = "aura", .palette = "neon_night", .seed = 1337, .speed = 0.12f,
            .frequency = 2.4f, .amplitude = 1.0f, .scale = 1.2f, .contrast = 1.0f, .softness = 0.8f, .grain_amount = 0.02f,
            .color_a = ColorSpec{7, 10, 18, 255}, .color_b = ColorSpec{6, 182, 212, 255}, .color_c = ColorSpec{236, 72, 153, 255}
        }},
        {"dot_grid_fade", "Dot Grid Fade", {
            .kind = "grid", .palette = "dark_tech", .seed = 2468, .speed = 0.2f,
            .frequency = 10.0f, .softness = 0.2f, .grain_amount = 0.0f, .shape = "circle", .spacing = 24.0f, .border_width = 1.1f,
            .color_a = ColorSpec{10, 10, 20, 255}, .color_b = ColorSpec{94, 63, 183, 255}, .color_c = ColorSpec{31, 41, 55, 255}
        }},
        {"noise_grain_vignette", "Noise Grain Vignette", {
            .kind = "noise", .palette = "premium_dark", .seed = 8080, .speed = 0.08f,
            .frequency = 14.0f, .amplitude = 0.3f, .scale = 1.8f, .contrast = 0.95f, .softness = 0.65f, .grain_amount = 0.08f,
            .color_a = ColorSpec{8, 9, 14, 255}, .color_b = ColorSpec{28, 31, 44, 255}
        }},
        {"mesh_gradient", "Mesh Gradient", {
            .kind = "aura", .palette = "neon_night", .seed = 2024, .speed = 0.16f,
            .frequency = 1.4f, .amplitude = 1.0f, .scale = 1.0f, .contrast = 1.0f, .softness = 0.45f, .grain_amount = 0.01f,
            .color_a = ColorSpec{124, 58, 237, 255}, .color_b = ColorSpec{6, 182, 212, 255}, .color_c = ColorSpec{236, 72, 153, 255}
        }},
        {"shape_grid_square", "Shape Grid Square", {
            .kind = "grid", .palette = "dark_tech", .seed = 1010, .shape = "square", .spacing = 46.0f, .border_width = 1.0f,
            .color_a = ColorSpec{14, 11, 18, 255}, .color_b = ColorSpec{38, 34, 51, 255}, .color_c = ColorSpec{26, 22, 35, 255}
        }},
        {"dark_grain", "Dark Grain", {
            .kind = "noise", .palette = "premium_dark", .contrast = 1.0f, .grain_amount = 0.015f,
            .color_a = ColorSpec{10, 10, 10, 255}
        }},
        {"white_paper", "White Paper", {
            .kind = "noise", .palette = "clean_white", .contrast = 1.0f, .grain_amount = 0.008f,
            .color_a = ColorSpec{250, 250, 250, 255}
        }},
        {"grid_dark", "Grid Dark", {
            .kind = "grid", .palette = "dark_tech", .softness = 0.0f, .spacing = 12.0f, .border_width = 1.0f,
            .color_a = ColorSpec{5, 5, 5, 255}, .color_b = ColorSpec{255, 255, 255, 18}
        }},
        {"grid_white", "Grid White", {
            .kind = "grid", .palette = "clean_white", .softness = 0.0f, .spacing = 12.0f, .border_width = 1.0f,
            .color_a = ColorSpec{252, 252, 252, 255}, .color_b = ColorSpec{0, 0, 0, 15}
        }},
        {"soft_mesh", "Soft Mesh", {
            .kind = "aura", .palette = "premium_dark", .frequency = 0.5f, .softness = 0.8f,
            .color_a = ColorSpec{14, 14, 14, 255}, .color_b = ColorSpec{23, 23, 23, 255}
        }},
        {"ripple_grid", "Ripple Grid", {
            .kind = "ripple_grid", .palette = "neon_ripple", .spacing = 10.0f, .border_width = 22.0f,
            .color_a = ColorSpec{106, 59, 255, 255}
        }},
        {"apple_blobs", "Apple Blobs", {
            .kind = "aura", .palette = "neon_night", .speed = 0.4f, .scale = 1.5f, .softness = 0.9f,
            .color_a = ColorSpec{255, 126, 182, 180}, .color_b = ColorSpec{139, 123, 255, 180}, .color_c = ColorSpec{77, 184, 255, 180}
        }},
        {"grid_bg", "Grid BG", {
            .kind = "grid", .palette = "dark_tech", .softness = 0.0f, .spacing = 100.0f, .border_width = 1.0f,
            .color_a = ColorSpec{13, 13, 13, 255}, .color_b = ColorSpec{30, 30, 30, 255}
        }},
        // Aliases for tests
        {"mesh_gradient_from_output_shader", "Mesh Gradient (Legacy)", {
            .kind = "aura", .palette = "neon_night", .seed = 2024, .speed = 0.16f,
            .color_a = ColorSpec{124, 58, 237, 255}, .color_b = ColorSpec{6, 182, 212, 255}, .color_c = ColorSpec{236, 72, 153, 255}
        }},
        {"dot_grid_fade_from_output_shader", "Dot Grid Fade (Legacy)", {
            .kind = "grid", .palette = "dark_tech", .seed = 2468, .speed = 0.2f,
            .color_a = ColorSpec{10, 10, 20, 255}, .color_b = ColorSpec{94, 63, 183, 255}, .color_c = ColorSpec{31, 41, 55, 255}
        }},
        {"liquid_glass_blobs_from_output_cpp", "Liquid Glass Blobs (Legacy)", {
            .kind = "aura", .palette = "neon_night", .seed = 1337, .speed = 0.12f,
            .color_a = ColorSpec{7, 10, 18, 255}, .color_b = ColorSpec{6, 182, 212, 255}, .color_c = ColorSpec{236, 72, 153, 255}
        }},
    };
    return defs;
}

} // namespace

std::optional<LayerSpec> build_background_preset(std::string_view id, int width,
                                                 int height) {
    if (id == "blank_canvas") {
        LayerSpec bg = make_base_layer("bg_blank_canvas", "Blank Canvas", "solid", {
            0.0, 5.0, 0.0, 0.0, static_cast<float>(width), static_cast<float>(height), 1.0f
        });
        bg.fill_color.value = ColorSpec{16, 16, 16, 255};
        return bg;
    }

    for (const auto& def : get_preset_defs()) {
        if (def.id == id) {
            auto params = def.params;
            params.w = static_cast<double>(width);
            params.h = static_cast<double>(height);
            params.in_point = 0.0;
            params.out_point = 8.0;

            auto layer = build_background(params);
            layer.id = "bg_" + std::string(id);
            layer.name = def.name;
            layer.preset_id = std::string(id);
            return layer;
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
