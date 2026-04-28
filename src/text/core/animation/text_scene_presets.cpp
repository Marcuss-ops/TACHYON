#include "tachyon/text/animation/text_scene_presets.h"

namespace tachyon::text {

namespace {

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
    LayerSpec bg;
    bg.id = "bg_procedural";
    bg.name = "Background";
    bg.type = "procedural";
    bg.enabled = true;
    bg.visible = true;
    bg.start_time = 0.0;
    bg.in_point = 0.0;
    bg.out_point = options.duration_seconds;
    bg.width = options.width;
    bg.height = options.height;
    bg.opacity = 1.0;
    bg.procedural = ProceduralSpec{};
    bg.procedural->kind = "aura";
    bg.procedural->seed = 19;
    bg.procedural->color_a = AnimatedColorSpec(options.procedural_color_a);
    bg.procedural->color_b = AnimatedColorSpec(options.procedural_color_b);
    bg.procedural->color_c = AnimatedColorSpec(options.procedural_color_c);
    bg.procedural->speed = AnimatedScalarSpec(options.procedural_speed);
    bg.procedural->frequency = AnimatedScalarSpec(options.procedural_frequency);
    bg.procedural->amplitude = AnimatedScalarSpec(options.procedural_amplitude);
    bg.procedural->scale = AnimatedScalarSpec(options.procedural_scale);
    bg.procedural->grain_amount = AnimatedScalarSpec(options.procedural_grain);
    bg.procedural->contrast = AnimatedScalarSpec(1.0);
    bg.procedural->gamma = AnimatedScalarSpec(1.0);
    bg.procedural->saturation = AnimatedScalarSpec(1.0);
    return bg;
}

LayerSpec make_enhance_text_layer(const TextScenePresetOptions& options) {
    LayerSpec text;
    text.id = "headline";
    text.name = "Headline";
    text.type = "text";
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
    scene.spec_version = "1.0";
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
