#include "tachyon/text/animation/text_scene_presets.h"
#include "tachyon/presets/background/background_builders.h"

namespace tachyon::text {

namespace {

::tachyon::presets::BackgroundParams make_background_params(const TextScenePresetOptions& options) {
    ::tachyon::presets::BackgroundParams params;
    params.kind = options.procedural_kind;
    params.in_point = 0.0;
    params.out_point = options.duration_seconds;
    params.x = 0.0f;
    params.y = 0.0f;
    params.w = static_cast<float>(options.width);
    params.h = static_cast<float>(options.height);
    params.opacity = 1.0f;
    params.seed = 19;
    params.speed = static_cast<float>(options.procedural_speed);
    params.color_a = options.procedural_color_a;
    params.color_b = options.procedural_color_b;
    params.color_c = options.procedural_color_c;
    params.frequency = static_cast<float>(options.procedural_frequency);
    params.amplitude = static_cast<float>(options.procedural_amplitude);
    params.scale = static_cast<float>(options.procedural_scale);
    params.grain_amount = static_cast<float>(options.procedural_grain);
    params.contrast = 1.0f;
    params.softness = 0.0f;

    if (options.procedural_kind == "tachyon.background.kind.grid") {
        params.kind = "tachyon.background.kind.grid";
        params.color_a = options.shape_grid_params.background_color;
        params.color_b = options.shape_grid_params.grid_color;
        params.color_c = std::nullopt;
        params.seed = options.shape_grid_params.seed;
        params.speed = options.shape_grid_params.speed;
        params.shape = options.shape_grid_params.shape;
        params.spacing = options.shape_grid_params.spacing;
        params.border_width = options.shape_grid_params.border_width;
    }

    return params;
}

TextAnimatorSpec make_default_fade_up() {
    return make_minimal_fade_up_animator("characters_excluding_spaces", 0.60, 18.0);
}

TextAnimatorSpec make_default_blur_focus() {
    return make_blur_to_focus_animator("characters_excluding_spaces", 0.55, 8.0);
}

TextAnimatorSpec make_default_soft_scale() {
    return make_soft_scale_in_animator("characters_excluding_spaces", 0.60, 0.96);
}

} // namespace

LayerSpec make_enhance_text_background_layer(const TextScenePresetOptions& options) {
    return ::tachyon::presets::build_background(make_background_params(options));
}

LayerSpec make_enhance_text_layer(const TextScenePresetOptions& options) {
    LayerSpec text;
    text.id = "headline";
    text.name = "Headline";
    text.type = LayerType::Text;
    text.type_string = "text";
    text.enabled = true;
    text.visible = true;
    text.start_time = 0.0;
    text.in_point = 0.0;
    text.out_point = options.duration_seconds;
    text.width = static_cast<int>(options.text_width);
    text.height = static_cast<int>(options.text_height);
    text.alignment = "center";
    text.text_content = options.text;
    text.font_id = options.font_id;
    text.font_size.value = static_cast<double>(options.font_size);
    text.fill_color = AnimatedColorSpec(ColorSpec{238, 242, 248, 245});
    text.transform.position_x = static_cast<double>(options.text_position_x);
    text.transform.position_y = static_cast<double>(options.text_position_y);

    if (!options.text_animators.empty()) {
        text.text_animators = options.text_animators;
    } else {
        text.text_animators = {
            make_default_fade_up(),
            make_default_blur_focus(),
            make_default_soft_scale()
        };
    }

    return text;
}

SceneSpec make_enhance_text_scene(const TextScenePresetOptions& options) {
    SceneSpec scene;
    scene.project.id = options.project_id;
    scene.project.name = options.project_name;

    CompositionSpec comp;
    comp.id = options.scene_id;
    comp.name = options.scene_name;
    comp.width = options.width;
    comp.height = options.height;
    comp.duration = options.duration_seconds;
    comp.frame_rate = options.frame_rate;
    comp.background = options.clear_background;

    if (options.use_procedural_background) {
        comp.layers.push_back(make_enhance_text_background_layer(options));
    }
    comp.layers.push_back(make_enhance_text_layer(options));

    scene.compositions.push_back(std::move(comp));
    return scene;
}

SceneSpec make_minimal_text_scene(const TextScenePresetOptions& options) {
    return make_enhance_text_scene(options);
}

} // namespace tachyon::text
