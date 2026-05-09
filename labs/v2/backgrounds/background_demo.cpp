#include "tachyon/scene/builder.h"
#include "tachyon/presets/background/fluent.h"
#include <cstdlib>
#include <string>

extern "C" void build_scene(tachyon::SceneSpec& out) {
    using namespace tachyon::scene;
    using namespace tachyon::presets::background;

    std::string bg_id = std::getenv("TACHYON_BACKGROUND_ID") ? std::getenv("TACHYON_BACKGROUND_ID") : "tachyon.background.kind.aura";
    double duration = std::getenv("TACHYON_DURATION") ? std::stod(std::getenv("TACHYON_DURATION")) : 5.0;

    out = SceneBuilder()
        .project("background_test", "Background Test")
        .composition("main", [bg_id, duration](CompositionBuilder& c) {
            c.size(1920, 1080)
             .fps(30)
             .duration(duration)
             .background(tachyon::BackgroundSpec::from_string("#000000"));  // Black background as base

            // Add the specific background layer
            if (bg_id == "tachyon.background.solid") {
                c.layer(solid({255, 255, 255, 255}, 1920, 1080, duration));
            } else if (bg_id == "tachyon.background.linear_gradient") {
                c.layer(solid({200, 200, 200, 255}, 1920, 1080, duration));  // Placeholder
            } else if (bg_id == "tachyon.background.radial_gradient") {
                c.layer(solid({150, 150, 150, 255}, 1920, 1080, duration));  // Placeholder
            } else if (bg_id == "tachyon.background.image") {
                c.layer(solid({100, 100, 100, 255}, 1920, 1080, duration));  // Placeholder
            } else if (bg_id == "tachyon.background.video") {
                c.layer(solid({50, 50, 50, 255}, 1920, 1080, duration));  // Placeholder
            } else if (bg_id == "tachyon.background.kind.aura") {
                c.layer(kind_aura(procedural_bg::palettes::neon_night())
                    .width(1920)
                    .height(1080)
                    .duration(duration)
                    .build());
            } else if (bg_id == "tachyon.background.kind.grid_modern") {
                c.layer(kind_grid_modern(procedural_bg::palettes::dark_tech())
                    .width(1920)
                    .height(1080)
                    .duration(duration)
                    .build());
            } else if (bg_id == "tachyon.background.kind.stars") {
                c.layer(kind_stars(procedural_bg::palettes::cool_mint())
                    .width(1920)
                    .height(1080)
                    .duration(duration)
                    .build());
            } else if (bg_id == "tachyon.background.kind.galaxy") {
                c.layer(kind_aura(procedural_bg::palettes::cosmic_nebula())  // Placeholder for galaxy
                    .width(1920)
                    .height(1080)
                    .duration(duration)
                    .build());
            } else if (bg_id == "tachyon.backgroundpreset.galaxy_premium") {
                c.layer(kind_aura(procedural_bg::palettes::cosmic_nebula())
                    .width(1920)
                    .height(1080)
                    .duration(duration)
                    .build());
            } else if (bg_id == "tachyon.backgroundpreset.dark_tech_grid") {
                c.layer(kind_grid_modern(procedural_bg::palettes::dark_tech())
                    .width(1920)
                    .height(1080)
                    .duration(duration)
                    .build());
            } else if (bg_id == "tachyon.backgroundpreset.cinematic_aura") {
                c.layer(kind_aura(procedural_bg::palettes::neon_night())
                    .width(1920)
                    .height(1080)
                    .duration(duration)
                    .softness(0.5)
                    .build());
            } else {
                // Default to aura
                c.layer(kind_aura(procedural_bg::palettes::neon_night())
                    .width(1920)
                    .height(1080)
                    .duration(duration)
                    .build());
            }
        })
        .build();
}