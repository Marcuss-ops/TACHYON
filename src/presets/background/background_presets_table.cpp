#include "tachyon/presets/background/background_preset_registry.h"
#include "tachyon/presets/background/procedural.h"
#include "tachyon/presets/builders_common.h"

namespace tachyon::presets {

namespace {

using namespace tachyon::presets::background::procedural_bg;

void register_procedural(BackgroundPresetRegistry& registry, std::string id, std::string name, std::function<ProceduralSpec()> factory) {
    registry.register_preset({
        std::move(id),
        std::move(name),
        "Premium procedural background.",
        [factory](int width, int height, double duration) {
            LayerSpec bg = make_base_layer("bg_layer", "Procedural", "procedural", {
                0.0, duration, 0.0, 0.0, static_cast<float>(width), static_cast<float>(height), 1.0f
            });
            bg.procedural = factory();
            return bg;
        }
    });
}

} // namespace

void BackgroundPresetRegistry::load_builtins() {
    // 1. Solid Colors
    register_preset({
        "blank_canvas", "Blank Canvas", "Flat dark background",
        [](int width, int height, double duration) {
            LayerSpec bg = make_base_layer("bg_blank_canvas", "Blank Canvas", "solid", {
                0.0, duration, 0.0, 0.0, static_cast<float>(width), static_cast<float>(height), 1.0f
            });
            bg.fill_color.value = ColorSpec{16, 16, 16, 255};
            return bg;
        }
    });

    register_preset({
        "solid_white", "Solid White", "Flat pure white background",
        [](int width, int height, double duration) {
            LayerSpec bg = make_base_layer("bg_solid", "Solid White", "solid", {
                0.0, duration, 0.0, 0.0, static_cast<float>(width), static_cast<float>(height), 1.0f
            });
            bg.fill_color.value = ColorSpec{255, 255, 255, 255};
            return bg;
        }
    });

    // 2. Procedural Premium Backgrounds
    register_procedural(*this, "midnight_silk", "Midnight Silk", []() { return make_midnight_silk_spec(); });
    register_procedural(*this, "cosmic_nebula", "Cosmic Nebula", []() { return make_cosmic_nebula_spec(); });
    register_procedural(*this, "cyber_matrix", "Cyber Matrix", []() { return make_cyber_matrix_spec(); });
    register_procedural(*this, "golden_horizon", "Golden Horizon", []() { return make_golden_horizon_spec(); });
    register_procedural(*this, "frosted_glass", "Frosted Glass", []() { return make_frosted_glass_spec(); });
    register_procedural(*this, "oceanic_abyss", "Oceanic Abyss", []() { return make_oceanic_abyss_spec(); });
    register_procedural(*this, "neon_night_aura", "Neon Night Aura", []() { return make_aura_spec(palettes::neon_night()); });
    register_procedural(*this, "clean_grid", "Clean Grid", []() { return make_clean_grid_spec(); });
    register_procedural(*this, "subtle_noise", "Subtle Noise", []() { return make_subtle_texture_spec(); });
    register_procedural(*this, "ripple_grid", "Ripple Grid", []() { return make_ripple_grid_spec(); });
}

} // namespace tachyon::presets
