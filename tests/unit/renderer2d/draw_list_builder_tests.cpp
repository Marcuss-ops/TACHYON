#include "tachyon/renderer2d/raster/draw_list_builder.h"

#include <iostream>
#include <string>

namespace {

int g_failures = 0;

void check_true(bool condition, const std::string& message) {
    if (!condition) {
        ++g_failures;
        std::cerr << "FAIL: " << message << '\n';
    }
}

tachyon::scene::EvaluatedCompositionState make_base_composition() {
    tachyon::scene::EvaluatedCompositionState composition;
    composition.composition_id = "main";
    composition.width = 1920;
    composition.height = 1080;
    composition.composition_time_seconds = 0.0;
    return composition;
}

tachyon::scene::EvaluatedLayerState make_layer(
    const std::string& id,
    tachyon::scene::LayerType type,
    float x,
    float y,
    float scale_x,
    float scale_y,
    double opacity) {

    tachyon::scene::EvaluatedLayerState layer;
    layer.id = id;
    layer.type = type;
    layer.enabled = true;
    layer.active = true;
    layer.visible = true;
    layer.opacity = static_cast<float>(opacity);
    layer.local_transform.position = {x, y};
    layer.local_transform.scale = {scale_x, scale_y};
    return layer;
}

} // namespace

bool run_draw_list_builder_tests() {
    g_failures = 0;

    {
        auto composition = make_base_composition();
        composition.layers.push_back(make_layer("solid_01", tachyon::scene::LayerType::Solid, 10.0f, 20.0f, 2.0f, 3.0f, 0.5));
        composition.layers.push_back(make_layer("shape_01", tachyon::scene::LayerType::Shape, 30.0f, 40.0f, 1.0f, 1.0f, 0.6));
        composition.layers.push_back(make_layer("mask_01", tachyon::scene::LayerType::Mask, 45.0f, 55.0f, 1.0f, 1.0f, 1.0));
        composition.layers.push_back(make_layer("image_01", tachyon::scene::LayerType::Image, 50.0f, 60.0f, 1.0f, 1.0f, 0.75));
        composition.layers.push_back(make_layer("text_01", tachyon::scene::LayerType::Text, 100.0f, 120.0f, 1.0f, 2.0f, 0.9));

        const auto draw_list = tachyon::renderer2d::DrawListBuilder::build(composition);
        check_true(draw_list.commands.size() == 6, "draw list should include one clear command and five layer commands");
        check_true(draw_list.commands[0].kind == tachyon::renderer2d::DrawCommandKind::Clear, "first command should be clear");
        check_true(draw_list.commands[1].kind == tachyon::renderer2d::DrawCommandKind::SolidRect, "solid layer should map to solid rect command");
        check_true(draw_list.commands[2].kind == tachyon::renderer2d::DrawCommandKind::SolidRect, "shape layer should map to solid rect command");
        check_true(draw_list.commands[3].kind == tachyon::renderer2d::DrawCommandKind::MaskRect, "mask layer should map to mask rect command");
        check_true(draw_list.commands[4].kind == tachyon::renderer2d::DrawCommandKind::TexturedQuad, "image layer should map to textured quad command");
        check_true(draw_list.commands[5].kind == tachyon::renderer2d::DrawCommandKind::TexturedQuad, "text layer should map to textured quad command");

        check_true(draw_list.commands[1].solid_rect.has_value(), "solid rect command should carry solid rect payload");
        check_true(draw_list.commands[1].solid_rect->rect.width == 200, "solid rect width should scale with layer scale x");
        check_true(draw_list.commands[1].solid_rect->rect.height == 300, "solid rect height should scale with layer scale y");
        check_true(draw_list.commands[1].solid_rect->opacity == 0.5f, "solid rect opacity should propagate from layer state");

        check_true(draw_list.commands[4].textured_quad.has_value(), "image command should carry textured quad payload");
        check_true(draw_list.commands[4].textured_quad->texture.id == "image:image_01", "image textured quad should use deterministic image texture handle");
        check_true(draw_list.commands[4].textured_quad->opacity == 0.75f, "image opacity should propagate to textured quad");

        check_true(draw_list.commands[5].textured_quad.has_value(), "text command should carry textured quad payload");
        check_true(draw_list.commands[5].textured_quad->texture.id == "text:text_01", "text textured quad should use deterministic text texture handle");
        check_true(draw_list.commands[5].textured_quad->opacity == 0.9f, "text opacity should propagate to textured quad");
    }

    {
        auto composition = make_base_composition();
        composition.layers.push_back(make_layer("top", tachyon::scene::LayerType::Image, 0.0f, 0.0f, 1.0f, 1.0f, 1.0));
        composition.layers.push_back(make_layer("bottom", tachyon::scene::LayerType::Solid, 0.0f, 0.0f, 1.0f, 1.0f, 1.0));

        const auto draw_list = tachyon::renderer2d::DrawListBuilder::build(composition);
        check_true(draw_list.commands.size() == 3, "draw list should keep one clear and two layer commands");
        check_true(draw_list.commands[1].z_order == 0, "first layer command should preserve z order 0");
        check_true(draw_list.commands[2].z_order == 1, "second layer command should preserve z order 1");
    }

    return g_failures == 0;
}
