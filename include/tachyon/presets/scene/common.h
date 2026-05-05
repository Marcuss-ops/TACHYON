#pragma once

#include "tachyon/core/spec/schema/objects/scene_spec.h"
#include "tachyon/presets/background/background_builders.h"
#include "tachyon/presets/image/image_builders.h"
#include "tachyon/presets/text/fluent.h"
#include "tachyon/presets/scene/scene_params.h"
#include "tachyon/scene/builder.h"

namespace tachyon::presets::scene {
using namespace tachyon::scene;

/**
 * Creates a minimal SceneSpec with the given parameters.
 */
inline SceneSpec minimal(const SceneParams& params) {
    SceneSpec scene;
    scene.project.name = params.name;

    CompositionSpec comp;
    comp.id = "main";
    comp.name = "Main Composition";
    comp.width = params.width;
    comp.height = params.height;
    comp.duration = params.duration;

    scene.compositions.push_back(std::move(comp));
    return scene;
}

/**
 * Creates a minimal SceneSpec with an optional background layer.
 */
inline SceneSpec minimal(const EnhancedSceneParams& params) {
    SceneSpec scene;
    scene.project.name = params.name;

    CompositionSpec comp;
    comp.id = "main";
    comp.name = "Main Composition";
    comp.width = params.width;
    comp.height = params.height;
    comp.duration = params.duration;

    if (params.background.has_value()) {
        comp.layers.push_back(tachyon::presets::build_background(*params.background));
    }

    scene.compositions.push_back(std::move(comp));
    return scene;
}

/**
 * Creates an enhanced SceneSpec with advanced options.
 */
inline SceneSpec enhance(const EnhancedSceneParams& params) {
    SceneSpec scene;
    scene.project.name = params.name;

    // Register assets
    for (const auto& asset : params.assets) {
        scene.assets.push_back(asset);
    }

    CompositionSpec comp;
    comp.id = "main";
    comp.name = "Main Composition";
    comp.width = params.width;
    comp.height = params.height;
    comp.duration = params.duration;

    // Background
    if (params.background.has_value()) {
        comp.layers.push_back(tachyon::presets::build_background(*params.background));
    }

    // Main Content (e.g., Image)
    if (params.main_asset_id.has_value()) {
        LayerSpec layer;
        layer.id = "main_content";
        layer.type = "image";
        layer.asset_id = *params.main_asset_id;
        layer.width = params.width;
        layer.height = params.height;
        comp.layers.push_back(std::move(layer));
    }

    scene.compositions.push_back(std::move(comp));
    return scene;
}

inline SceneSpec build_enhanced_text_scene() {
    EnhancedSceneParams params;
    params.name = "Enhanced Text Scene";
    params.width = 1920;
    params.height = 1080;
    params.duration = 5.0;
    params.main_text = "Hello Tachyon";
    return enhance(params);
}

inline SceneSpec build_modern_grid_scene() {
    EnhancedSceneParams params;
    params.name = "Modern Grid Scene";
    params.width = 1920;
    params.height = 1080;
    params.duration = 5.0;
    params.background = BackgroundParams{};
    params.background->kind = "color";
    return minimal(params);
}

inline SceneSpec build_classico_premium_scene() {
    EnhancedSceneParams params;
    params.name = "Classico Premium";
    params.width = 1920;
    params.height = 1080;
    params.duration = 5.0;
    params.background = BackgroundParams{};
    params.background->kind = "color";
    return minimal(params);
}

inline SceneSpec build_minimal_text_scene() {
    EnhancedSceneParams params;
    params.name = "Minimal Text Scene";
    params.width = 1920;
    params.height = 1080;
    params.duration = 5.0;
    params.background = BackgroundParams{};
    params.background->kind = "color";
    return minimal(params);
}

inline SceneSpec build_scene_a() {
    return Composition("main")
        .size(1920, 1080)
        .duration(5.0)
        .fps(30)
        .clear(ColorSpec{25, 25, 204, 255}) // Blue
        .layer(tachyon::presets::text::headline("TESTO SCENA A")
            .font("Default")
            .font_size(120)
            .color(ColorSpec{255, 255, 255, 255})
            .stroke_color(ColorSpec{0, 0, 0, 255})
            .stroke_width(6.0)
            .center()
            .text_box(1920, 1080)
            .position(960, 540)
            .duration(5.0))
        .build_scene();
}

inline SceneSpec build_scene_b() {
    return Composition("main")
        .size(1920, 1080)
        .duration(5.0)
        .fps(30)
        .clear(ColorSpec{204, 25, 25, 255}) // Red
        .layer(tachyon::presets::text::headline("TESTO SCENA B")
            .font("Default")
            .font_size(120)
            .color(ColorSpec{255, 255, 255, 255})
            .stroke_color(ColorSpec{0, 0, 0, 255})
            .stroke_width(6.0)
            .center()
            .text_box(1920, 1080)
            .position(960, 540)
            .duration(5.0))
        .build_scene();
}

} // namespace tachyon::presets::scene
