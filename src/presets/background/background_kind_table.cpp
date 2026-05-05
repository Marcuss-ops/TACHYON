#include "tachyon/presets/background/background_kind_registry.h"
#include "tachyon/presets/background/procedural.h"

namespace tachyon::presets {

namespace {

using namespace tachyon::presets::background::procedural_bg;

void register_procedural_kind(
    BackgroundKindRegistry& registry,
    std::string id,
    std::string name,
    std::string description,
    std::function<ProceduralSpec(const ProceduralParams&, const BackgroundParams&)> factory) {
    registry.register_spec({
        id,
        {id, name, description, "background.kind", {"procedural", "background"}},
        std::move(factory)
    });
}

} // namespace

void BackgroundKindRegistry::load_builtins() {
    register_procedural_kind(*this, "tachyon.background.kind.grid_modern", "Modern Tech Grid", "Modern grid with scanline energy.",
        [](const ProceduralParams& palette, const BackgroundParams&) {
            return make_modern_tech_grid_spec(palette, 60.0);
        });

    register_procedural_kind(*this, "tachyon.background.kind.grid", "Clean Grid", "Clean geometric grid.",
        [](const ProceduralParams& palette, const BackgroundParams&) {
            return make_clean_grid_spec(palette);
        });

    register_procedural_kind(*this, "tachyon.background.kind.aura", "Aura", "Cinematic aura background.",
        [](const ProceduralParams& palette, const BackgroundParams&) {
            return make_aura_spec(palette);
        });

    register_procedural_kind(*this, "tachyon.background.kind.stars", "Stars", "Sparse star field.",
        [](const ProceduralParams& palette, const BackgroundParams&) {
            return make_stars_spec(palette);
        });

    register_procedural_kind(*this, "tachyon.background.kind.noise", "Noise", "Procedural noise texture.",
        [](const ProceduralParams& palette, const BackgroundParams&) {
            return make_noise_spec(palette);
        });

    register_procedural_kind(*this, "tachyon.background.kind.texture", "Texture", "Subtle texture layer.",
        [](const ProceduralParams& palette, const BackgroundParams&) {
            return make_subtle_texture_spec(palette);
        });

    register_procedural_kind(*this, "tachyon.background.kind.soft_gradient", "Soft Gradient", "Soft gradient wash.",
        [](const ProceduralParams& palette, const BackgroundParams&) {
            return make_soft_gradient_spec(palette);
        });

    register_procedural_kind(*this, "tachyon.background.kind.dots", "Dots", "Minimal dotted field.",
        [](const ProceduralParams& palette, const BackgroundParams&) {
            return make_elegant_dots_spec(palette);
        });

    register_procedural_kind(*this, "tachyon.background.kind.silk", "Midnight Silk", "Dark cinematic silk.",
        [](const ProceduralParams& palette, const BackgroundParams&) {
            return make_midnight_silk_spec(palette);
        });

    register_procedural_kind(*this, "tachyon.background.kind.horizon", "Golden Horizon", "Warm horizon glow.",
        [](const ProceduralParams& palette, const BackgroundParams&) {
            return make_golden_horizon_spec(palette);
        });

    register_procedural_kind(*this, "tachyon.background.kind.cyber", "Cyber Matrix", "Cyber grid matrix.",
        [](const ProceduralParams& palette, const BackgroundParams&) {
            return make_cyber_matrix_spec(palette);
        });

    register_procedural_kind(*this, "tachyon.background.kind.glass", "Frosted Glass", "Frosted glass noise layer.",
        [](const ProceduralParams& palette, const BackgroundParams&) {
            return make_frosted_glass_spec(palette);
        });

    register_procedural_kind(*this, "tachyon.background.kind.nebula", "Cosmic Nebula", "Nebula-like aura.",
        [](const ProceduralParams& palette, const BackgroundParams&) {
            return make_cosmic_nebula_spec(palette);
        });

    register_procedural_kind(*this, "tachyon.background.kind.metal", "Brushed Metal", "Brushed metallic surface.",
        [](const ProceduralParams& palette, const BackgroundParams&) {
            return make_brushed_metal_spec(palette);
        });

    register_procedural_kind(*this, "tachyon.background.kind.ocean", "Oceanic Abyss", "Deep ocean atmosphere.",
        [](const ProceduralParams& palette, const BackgroundParams&) {
            return make_oceanic_abyss_spec(palette);
        });

    register_procedural_kind(*this, "tachyon.background.kind.ripple_grid", "Ripple Grid", "Rippled grid variant.",
        [](const ProceduralParams& palette, const BackgroundParams&) {
            return make_ripple_grid_spec(palette);
        });

    register_procedural_kind(*this, "tachyon.background.kind.classico_premium", "Classico Premium", "Fallback premium aura.",
        [](const ProceduralParams& palette, const BackgroundParams&) {
            return make_classico_premium_spec(palette);
        });
}

} // namespace tachyon::presets
