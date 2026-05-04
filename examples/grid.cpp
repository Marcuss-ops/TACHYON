#include "tachyon/scene/builder.h"

namespace {

tachyon::LayerSpec make_solid_layer(
    const std::string& id,
    float x,
    float y,
    float w,
    float h,
    const tachyon::ColorSpec& fill) {

    tachyon::LayerSpec layer;
    layer.id = id;
    layer.name = id;
    layer.type = "solid";
    layer.kind = tachyon::LayerType::Solid;
    layer.enabled = true;
    layer.visible = true;
    layer.width = static_cast<int>(w);
    layer.height = static_cast<int>(h);
    layer.transform.position_x = x;
    layer.transform.position_y = y;
    layer.fill_color = tachyon::AnimatedColorSpec{fill};
    return layer;
}

tachyon::LayerSpec make_grid_layer(int width, int height) {
    tachyon::LayerSpec layer;
    layer.id = "grid_bg";
    layer.name = "Grid Background";
    layer.type = "procedural";
    layer.kind = tachyon::LayerType::Procedural;
    layer.enabled = true;
    layer.visible = true;
    layer.width = width;
    layer.height = height;
    layer.opacity = 1.0;
    layer.procedural.emplace();
    layer.procedural->kind = "grid_lines";
    layer.procedural->seed = 42;
    layer.procedural->color_a = tachyon::AnimatedColorSpec{tachyon::ColorSpec{10, 8, 14, 255}};
    layer.procedural->color_b = tachyon::AnimatedColorSpec{tachyon::ColorSpec{58, 48, 78, 255}};
    layer.procedural->color_c = tachyon::AnimatedColorSpec{tachyon::ColorSpec{18, 15, 23, 255}};
    layer.procedural->spacing = tachyon::AnimatedScalarSpec{40.0};
    layer.procedural->border_width = tachyon::AnimatedScalarSpec{1.0};
    layer.procedural->speed = tachyon::AnimatedScalarSpec{0.0};
    layer.procedural->grain_amount = tachyon::AnimatedScalarSpec{0.0};
    layer.procedural->contrast = tachyon::AnimatedScalarSpec{1.0};
    layer.procedural->gamma = tachyon::AnimatedScalarSpec{1.0};
    layer.procedural->saturation = tachyon::AnimatedScalarSpec{1.0};
    layer.procedural->softness = tachyon::AnimatedScalarSpec{0.0};
    layer.blend_mode = "normal";
    return layer;
}

tachyon::LayerSpec make_vignette_layer(int width, int height) {
    tachyon::LayerSpec layer;
    layer.id = "vignette";
    layer.name = "Vignette";
    layer.type = "procedural";
    layer.kind = tachyon::LayerType::Procedural;
    layer.enabled = true;
    layer.visible = true;
    layer.width = width;
    layer.height = height;
    layer.opacity = 0.82;
    layer.blend_mode = "multiply";
    layer.procedural.emplace();
    layer.procedural->kind = "aura";
    layer.procedural->seed = 7;
    layer.procedural->color_a = tachyon::AnimatedColorSpec{tachyon::ColorSpec{0, 0, 0, 0}};
    layer.procedural->color_b = tachyon::AnimatedColorSpec{tachyon::ColorSpec{0, 0, 0, 0}};
    layer.procedural->color_c = tachyon::AnimatedColorSpec{tachyon::ColorSpec{18, 15, 23, 255}};
    layer.procedural->speed = tachyon::AnimatedScalarSpec{0.0};
    layer.procedural->frequency = tachyon::AnimatedScalarSpec{2.0};
    layer.procedural->amplitude = tachyon::AnimatedScalarSpec{0.5};
    layer.procedural->scale = tachyon::AnimatedScalarSpec{1.5};
    layer.procedural->grain_amount = tachyon::AnimatedScalarSpec{0.0};
    layer.procedural->contrast = tachyon::AnimatedScalarSpec{0.9};
    layer.procedural->gamma = tachyon::AnimatedScalarSpec{1.0};
    layer.procedural->saturation = tachyon::AnimatedScalarSpec{0.0};
    layer.procedural->softness = tachyon::AnimatedScalarSpec{0.95};
    return layer;
}

tachyon::LayerSpec make_text_layer(
    const std::string& id,
    const std::string& content,
    float x,
    float y,
    float w,
    float h,
    const tachyon::ColorSpec& color,
    double font_size) {

    tachyon::LayerSpec layer;
    layer.id = id;
    layer.name = id;
    layer.type = "text";
    layer.kind = tachyon::LayerType::Text;
    layer.enabled = true;
    layer.visible = true;
    layer.width = static_cast<int>(w);
    layer.height = static_cast<int>(h);
    layer.text_content = content;
    layer.font_size = tachyon::AnimatedScalarSpec{font_size};
    layer.fill_color = tachyon::AnimatedColorSpec{color};
    layer.alignment = "left";
    layer.transform.position_x = x;
    layer.transform.position_y = y;
    layer.transform.position_property = tachyon::AnimatedVector2Spec{tachyon::math::Vector2{x, y}};
    layer.transform.position_property.keyframes.push_back(
        tachyon::Vector2KeyframeSpec{0.0, tachyon::math::Vector2{x, y}});
    return layer;
}

} // namespace

extern "C" void build_scene(tachyon::SceneSpec& out) {
    using namespace tachyon;
    using namespace tachyon::scene;

    constexpr int kWidth = 1280;
    constexpr int kHeight = 720;

    out = SceneBuilder()
        .project("grid", "ShapeGrid")
        .composition("main", [=](CompositionBuilder& c) {
            c.size(kWidth, kHeight)
             .fps(60)
             .duration(5.0)
             .clear({18, 15, 23, 255});

            c.layer(make_grid_layer(kWidth, kHeight));
            c.layer(make_vignette_layer(kWidth, kHeight));

            c.layer(make_solid_layer("frame_top", 0.0f, 0.0f, 1280.0f, 1.0f, {69, 57, 91, 220}));
            c.layer(make_solid_layer("frame_bottom", 0.0f, 719.0f, 1280.0f, 1.0f, {69, 57, 91, 220}));
            c.layer(make_solid_layer("frame_left", 0.0f, 0.0f, 1.0f, 720.0f, {69, 57, 91, 220}));
            c.layer(make_solid_layer("frame_right", 1279.0f, 0.0f, 1.0f, 720.0f, {69, 57, 91, 220}));

            c.layer(make_solid_layer("badge", 1004.0f, 648.0f, 260.0f, 40.0f, {18, 15, 23, 220}));
            c.layer(make_text_layer(
                "badge_text",
                "Demo Content",
                1020.0f,
                654.0f,
                220.0f,
                28.0f,
                {117, 107, 128, 255},
                12.0));
        })
        .build();
}
