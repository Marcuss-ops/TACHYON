#pragma once

#include "tachyon/core/spec/schema/objects/scene_spec.h"
#include "tachyon/core/spec/schema/objects/composition_spec.h"
#include "tachyon/presets/text/layer.h"
#include "tachyon/presets/background/procedural.h"
#include "tachyon/core/types/colors.h"

namespace tachyon::presets::scene {

// Scene preset factory functions
// These create complete, ready-to-render scene specifications

struct SceneParams {
    std::string project_id = "project_default";
    std::string project_name = "Tachyon Project";
    std::string scene_id = "scene_main";
    std::string scene_name = "Main Scene";
    int width = 1920;
    int height = 1080;
    double duration_seconds = 5.0;
    double frame_rate = 30.0;
    ColorSpec clear_background{0, 0, 0, 0};
    bool use_procedural_background = true;
};

struct TextSceneParams : SceneParams {
    std::string text = "Hello World";
    std::string font_id;
    double font_size = 72.0;
    int text_width = 1920;
    int text_height = 200;
    int text_position_x = 960;
    int text_position_y = 540;
    ColorSpec text_fill_color{238, 242, 248, 245};
    std::vector<TextAnimatorSpec> text_animators;

    // Procedural background options
    std::string procedural_kind = "aura";
    ColorSpec procedural_color_a{5, 8, 20, 255};
    ColorSpec procedural_color_b{25, 30, 50, 255};
    ColorSpec procedural_color_c{0, 0, 0, 0};
    double procedural_speed = 1.0;
    double procedural_frequency = 3.0;
    double procedural_amplitude = 1.0;
    double procedural_scale = 1.0;
    double procedural_grain = 0.15;
    uint64_t procedural_seed = 19;
};

[[nodiscard]] inline SceneSpec build_enhanced_text_scene(const TextSceneParams& opts = {}) {
    SceneSpec scene;
    scene.schema_version = SchemaVersion{1, 0, 0};
    scene.project.id = opts.project_id;
    scene.project.name = opts.project_name;

    CompositionSpec comp;
    comp.id = opts.scene_id;
    comp.name = opts.scene_name;
    comp.width = opts.width;
    comp.height = opts.height;
    comp.duration = opts.duration_seconds;
    comp.frame_rate.numerator = static_cast<std::int64_t>(opts.frame_rate);
    comp.frame_rate.denominator = 1;
    comp.background = BackgroundSpec::from_string("rgba(0,0,0,0)");

    if (opts.use_procedural_background) {
        auto bg = background::procedural_bg::aura(
            opts.width, opts.height,
            {opts.procedural_color_a, opts.procedural_color_b,
             opts.procedural_color_c, opts.procedural_speed, 1.0,
             opts.procedural_grain, 0.0, opts.procedural_seed},
            opts.duration_seconds);
        comp.layers.push_back(bg);
    }

    text::TextParams text_opts;
    text_opts.id = "headline";
    text_opts.text = opts.text;
    text_opts.font_id = opts.font_id;
    text_opts.font_size = opts.font_size;
    text_opts.width = opts.text_width;
    text_opts.height = opts.text_height;
    text_opts.position_x = opts.text_position_x;
    text_opts.position_y = opts.text_position_y;
    text_opts.alignment = "center";
    text_opts.fill_color = opts.text_fill_color;
    text_opts.start_time = 0.0;
    text_opts.duration = opts.duration_seconds;
    text_opts.animators = opts.text_animators;

    comp.layers.push_back(text::build_enhanced(text_opts));
    scene.compositions.push_back(std::move(comp));
    return scene;
}

[[nodiscard]] inline SceneSpec build_minimal_text_scene(const TextSceneParams& opts = {}) {
    SceneSpec scene;
    scene.schema_version = SchemaVersion{1, 0, 0};
    scene.project.id = opts.project_id;
    scene.project.name = opts.project_name;

    CompositionSpec comp;
    comp.id = opts.scene_id;
    comp.name = opts.scene_name;
    comp.width = opts.width;
    comp.height = opts.height;
    comp.duration = opts.duration_seconds;
    comp.frame_rate.numerator = static_cast<std::int64_t>(opts.frame_rate);
    comp.frame_rate.denominator = 1;
    comp.background = BackgroundSpec::from_string("rgba(0,0,0,0)");

    if (opts.use_procedural_background) {
        auto bg = background::procedural_bg::aura(
            opts.width, opts.height,
            {opts.procedural_color_a, opts.procedural_color_b,
             opts.procedural_color_c, opts.procedural_speed, 1.0,
             opts.procedural_grain, 0.0, opts.procedural_seed},
            opts.duration_seconds);
        comp.layers.push_back(bg);
    }

    text::TextParams text_opts;
    text_opts.id = "headline";
    text_opts.text = opts.text;
    text_opts.font_id = opts.font_id;
    text_opts.font_size = opts.font_size;
    text_opts.width = opts.text_width;
    text_opts.height = opts.text_height;
    text_opts.position_x = opts.text_position_x;
    text_opts.position_y = opts.text_position_y;
    text_opts.alignment = "center";
    text_opts.fill_color = opts.text_fill_color;
    text_opts.start_time = 0.0;
    text_opts.duration = opts.duration_seconds;
    text_opts.animators = opts.text_animators;

    comp.layers.push_back(text::build_minimal(text_opts));
    scene.compositions.push_back(std::move(comp));
    return scene;
}

[[nodiscard]] inline SceneSpec build_modern_grid_scene(const TextSceneParams& opts = {}) {
    SceneSpec scene;
    scene.schema_version = SchemaVersion{1, 0, 0};
    scene.project.id = opts.project_id;
    scene.project.name = opts.project_name;

    CompositionSpec comp;
    comp.id = opts.scene_id;
    comp.name = opts.scene_name;
    comp.width = opts.width;
    comp.height = opts.height;
    comp.duration = opts.duration_seconds;
    comp.frame_rate.numerator = static_cast<std::int64_t>(opts.frame_rate);
    comp.frame_rate.denominator = 1;
    comp.background = BackgroundSpec::from_string("rgba(0,0,0,0)");

    auto bg = background::procedural_bg::modern_tech_grid(
        opts.width, opts.height,
        background::procedural_bg::palettes::neon_grid(),
        60.0,
        opts.duration_seconds);
    comp.layers.push_back(bg);

    text::TextParams text_opts;
    text_opts.id = "headline";
    text_opts.text = opts.text;
    text_opts.font_id = opts.font_id;
    text_opts.font_size = opts.font_size;
    text_opts.width = opts.text_width;
    text_opts.height = opts.text_height;
    text_opts.position_x = opts.text_position_x;
    text_opts.position_y = opts.text_position_y;
    text_opts.alignment = "center";
    text_opts.fill_color = opts.text_fill_color;
    text_opts.start_time = 0.0;
    text_opts.duration = opts.duration_seconds;
    text_opts.animators = opts.text_animators;

    comp.layers.push_back(text::build_enhanced(text_opts));
    scene.compositions.push_back(std::move(comp));
    return scene;
}

[[nodiscard]] inline SceneSpec build_classico_premium_scene(const TextSceneParams& opts = {}) {
    SceneSpec scene;
    scene.schema_version = SchemaVersion{1, 0, 0};
    scene.project.id = opts.project_id;
    scene.project.name = opts.project_name;

    CompositionSpec comp;
    comp.id = opts.scene_id;
    comp.name = opts.scene_name;
    comp.width = opts.width;
    comp.height = opts.height;
    comp.duration = opts.duration_seconds;
    comp.frame_rate.numerator = static_cast<std::int64_t>(opts.frame_rate);
    comp.frame_rate.denominator = 1;
    comp.background = BackgroundSpec::from_string("rgba(0,0,0,0)");

    auto bg = background::procedural_bg::classico_premium(
        opts.width, opts.height,
        background::procedural_bg::palettes::premium_dark(),
        opts.duration_seconds);
    comp.layers.push_back(bg);

    text::TextParams text_opts;
    text_opts.id = "headline";
    text_opts.text = opts.text;
    text_opts.font_id = opts.font_id;
    text_opts.font_size = opts.font_size;
    text_opts.width = opts.text_width;
    text_opts.height = opts.text_height;
    text_opts.position_x = opts.text_position_x;
    text_opts.position_y = opts.text_position_y;
    text_opts.alignment = "center";
    text_opts.fill_color = opts.text_fill_color;
    text_opts.start_time = 0.0;
    text_opts.duration = opts.duration_seconds;
    text_opts.animators = opts.text_animators;

    comp.layers.push_back(text::build_enhanced(text_opts));
    scene.compositions.push_back(std::move(comp));
    return scene;
}

[[nodiscard]] inline SceneSpec text_with_background(
    const std::string& preset_name,
    const TextSceneParams& opts = {}) {
    if (preset_name == "minimal") {
        return build_minimal_text_scene(opts);
    }
    return build_enhanced_text_scene(opts);
}

} // namespace tachyon::presets::scene
