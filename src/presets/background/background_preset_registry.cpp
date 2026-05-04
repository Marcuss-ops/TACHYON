#include "tachyon/presets/background/background_preset_registry.h"
#include "tachyon/presets/background/procedural.h"
#include "tachyon/presets/background/background_builders.h"
#include "tachyon/presets/builders_common.h"
#include <map>
#include <mutex>

namespace tachyon::presets {

struct BackgroundPresetRegistry::Impl {
    std::map<std::string, BackgroundPresetSpec, std::less<>> presets;
    std::mutex mutex;
};

BackgroundPresetRegistry& BackgroundPresetRegistry::instance() {
    static BackgroundPresetRegistry registry;
    return registry;
}

BackgroundPresetRegistry::BackgroundPresetRegistry() : m_impl(std::make_unique<Impl>()) {
    // Helper to register based on BackgroundParams
    auto reg_params = [this](std::string id, std::string name, BackgroundParams p) {
        register_preset({
            std::move(id),
            std::move(name),
            "",
            [p](int width, int height, double duration) {
                auto params = p;
                params.w = static_cast<double>(width);
                params.h = static_cast<double>(height);
                params.in_point = 0.0;
                params.out_point = duration;
                
                auto layer = build_background(params);
                // The caller might override these, but we set defaults here
                layer.id = "bg_layer"; 
                return layer;
            }
        });
    };

    // 1. Special case: blank_canvas
    register_preset({
        "blank_canvas",
        "Blank Canvas",
        "Flat solid background",
        [](int width, int height, double duration) {
            LayerSpec bg = make_base_layer("bg_blank_canvas", "Blank Canvas", "solid", {
                0.0, duration, 0.0, 0.0, static_cast<float>(width), static_cast<float>(height), 1.0f
            });
            bg.fill_color.value = ColorSpec{16, 16, 16, 255};
            return bg;
        }
    });

    // 2. Procedural backgrounds (from old get_preset_defs)
    reg_params("aurora_mesh", "Aurora Mesh", {
        .kind = "aura", .palette = "neon_night", .seed = 42, .speed = 0.18f,
        .frequency = 1.8f, .amplitude = 1.0f, .scale = 1.0f, .contrast = 1.08f, .softness = 0.55f, .grain_amount = 0.03f,
        .color_a = ColorSpec{5, 8, 20, 255}, .color_b = ColorSpec{0, 178, 204, 255}, .color_c = ColorSpec{192, 64, 255, 255}
    });

    reg_params("liquid_glass_blobs", "Liquid Glass Blobs", {
        .kind = "aura", .palette = "neon_night", .seed = 1337, .speed = 0.12f,
        .frequency = 2.4f, .amplitude = 1.0f, .scale = 1.2f, .contrast = 1.0f, .softness = 0.8f, .grain_amount = 0.02f,
        .color_a = ColorSpec{7, 10, 18, 255}, .color_b = ColorSpec{6, 182, 212, 255}, .color_c = ColorSpec{236, 72, 153, 255}
    });

    reg_params("dot_grid_fade", "Dot Grid Fade", {
        .kind = "grid", .palette = "dark_tech", .seed = 2468, .speed = 0.2f,
        .frequency = 10.0f, .softness = 0.2f, .grain_amount = 0.0f, .shape = "circle", .spacing = 24.0f, .border_width = 1.1f,
        .color_a = ColorSpec{10, 10, 20, 255}, .color_b = ColorSpec{94, 63, 183, 255}, .color_c = ColorSpec{31, 41, 55, 255}
    });

    reg_params("noise_grain_vignette", "Noise Grain Vignette", {
        .kind = "noise", .palette = "premium_dark", .seed = 8080, .speed = 0.08f,
        .frequency = 14.0f, .amplitude = 0.3f, .scale = 1.8f, .contrast = 0.95f, .softness = 0.65f, .grain_amount = 0.08f,
        .color_a = ColorSpec{8, 9, 14, 255}, .color_b = ColorSpec{28, 31, 44, 255}
    });

    reg_params("mesh_gradient", "Mesh Gradient", {
        .kind = "aura", .palette = "neon_night", .seed = 2024, .speed = 0.16f,
        .frequency = 1.4f, .amplitude = 1.0f, .scale = 1.0f, .contrast = 1.0f, .softness = 0.45f, .grain_amount = 0.01f,
        .color_a = ColorSpec{124, 58, 237, 255}, .color_b = ColorSpec{6, 182, 212, 255}, .color_c = ColorSpec{236, 72, 153, 255}
    });

    reg_params("shape_grid_square", "Shape Grid Square", {
        .kind = "grid", .palette = "dark_tech", .seed = 1010, .shape = "square", .spacing = 46.0f, .border_width = 1.0f,
        .color_a = ColorSpec{14, 11, 18, 255}, .color_b = ColorSpec{38, 34, 51, 255}, .color_c = ColorSpec{26, 22, 35, 255}
    });

    reg_params("dark_grain", "Dark Grain", {
        .kind = "noise", .palette = "premium_dark", .contrast = 1.0f, .grain_amount = 0.015f,
        .color_a = ColorSpec{10, 10, 10, 255}
    });

    reg_params("white_paper", "White Paper", {
        .kind = "noise", .palette = "clean_white", .contrast = 1.0f, .grain_amount = 0.008f,
        .color_a = ColorSpec{250, 250, 250, 255}
    });

    reg_params("grid_dark", "Grid Dark", {
        .kind = "grid", .palette = "dark_tech", .softness = 0.0f, .spacing = 12.0f, .border_width = 1.0f,
        .color_a = ColorSpec{5, 5, 5, 255}, .color_b = ColorSpec{255, 255, 255, 18}
    });

    reg_params("grid_white", "Grid White", {
        .kind = "grid", .palette = "clean_white", .softness = 0.0f, .spacing = 12.0f, .border_width = 1.0f,
        .color_a = ColorSpec{252, 252, 252, 255}, .color_b = ColorSpec{0, 0, 0, 15}
    });

    reg_params("soft_mesh", "Soft Mesh", {
        .kind = "aura", .palette = "premium_dark", .frequency = 0.5f, .softness = 0.8f,
        .color_a = ColorSpec{14, 14, 14, 255}, .color_b = ColorSpec{23, 23, 23, 255}
    });

    reg_params("ripple_grid", "Ripple Grid", {
        .kind = "ripple_grid", .palette = "neon_ripple", .spacing = 10.0f, .border_width = 22.0f,
        .color_a = ColorSpec{106, 59, 255, 255}
    });

    reg_params("apple_blobs", "Apple Blobs", {
        .kind = "aura", .palette = "neon_night", .speed = 0.4f, .scale = 1.5f, .softness = 0.9f,
        .color_a = ColorSpec{255, 126, 182, 180}, .color_b = ColorSpec{139, 123, 255, 180}, .color_c = ColorSpec{77, 184, 255, 180}
    });

    reg_params("grid_bg", "Grid BG", {
        .kind = "grid", .palette = "dark_tech", .softness = 0.0f, .spacing = 100.0f, .border_width = 1.0f,
        .color_a = ColorSpec{13, 13, 13, 255}, .color_b = ColorSpec{30, 30, 30, 255}
    });

    // Legacy Aliases
    reg_params("mesh_gradient_from_output_shader", "Mesh Gradient (Legacy)", {
        .kind = "aura", .palette = "neon_night", .seed = 2024, .speed = 0.16f,
        .color_a = ColorSpec{124, 58, 237, 255}, .color_b = ColorSpec{6, 182, 212, 255}, .color_c = ColorSpec{236, 72, 153, 255}
    });
    
    reg_params("dot_grid_fade_from_output_shader", "Dot Grid Fade (Legacy)", {
        .kind = "grid", .palette = "dark_tech", .seed = 2468, .speed = 0.2f,
        .color_a = ColorSpec{10, 10, 20, 255}, .color_b = ColorSpec{94, 63, 183, 255}, .color_c = ColorSpec{31, 41, 55, 255}
    });

    reg_params("liquid_glass_blobs_from_output_cpp", "Liquid Glass Blobs (Legacy)", {
        .kind = "aura", .palette = "neon_night", .seed = 1337, .speed = 0.12f,
        .color_a = ColorSpec{7, 10, 18, 255}, .color_b = ColorSpec{6, 182, 212, 255}, .color_c = ColorSpec{236, 72, 153, 255}
    });
}

void BackgroundPresetRegistry::register_preset(BackgroundPresetSpec spec) {
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    m_impl->presets[spec.id] = std::move(spec);
}

const BackgroundPresetSpec* BackgroundPresetRegistry::find(std::string_view id) const {
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    auto it = m_impl->presets.find(id);
    if (it != m_impl->presets.end()) {
        return &it->second;
    }
    return nullptr;
}

std::optional<LayerSpec> BackgroundPresetRegistry::create(std::string_view id, int width, int height, double duration) const {
    if (const auto* spec = find(id)) {
        auto layer = spec->factory(width, height, duration);
        layer.id = "bg_" + std::string(id);
        layer.name = spec->name;
        layer.preset_id = std::string(id);
        return layer;
    }
    return std::nullopt;
}

std::vector<std::string> BackgroundPresetRegistry::list_ids() const {
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    std::vector<std::string> ids;
    ids.reserve(m_impl->presets.size());
    for (const auto& [id, _] : m_impl->presets) {
        ids.push_back(id);
    }
    return ids;
}

// Compatibility wrappers
std::optional<LayerSpec> build_background_preset(std::string_view id, int width, int height) {
    return BackgroundPresetRegistry::instance().create(id, width, height);
}

std::vector<std::string> list_background_presets() {
    return BackgroundPresetRegistry::instance().list_ids();
}

} // namespace tachyon::presets
