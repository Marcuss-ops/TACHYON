#include "tachyon/renderer2d/draw_list_builder.h"
#include "tachyon/core/math/quaternion.h"

#include <cmath>
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

tachyon::scene::EvaluatedLayerState make_layer(const std::string& id, const std::string& type, float x, float y, float scale_x, float scale_y, double opacity) {
    tachyon::scene::EvaluatedLayerState layer;
    layer.id = id;
    layer.type = type;
    layer.name = id;
    layer.enabled = true;
    layer.active = true;
    layer.position = {x, y};
    layer.scale = {scale_x, scale_y};
    layer.opacity = opacity;
    return layer;
}

float quad_width(const tachyon::renderer2d::TexturedQuadCommand& quad) {
    const float dx = quad.p1.x - quad.p0.x;
    const float dy = quad.p1.y - quad.p0.y;
    return std::sqrt(dx * dx + dy * dy);
}

float quad_center_x(const tachyon::renderer2d::TexturedQuadCommand& quad) {
    return (quad.p0.x + quad.p1.x + quad.p2.x + quad.p3.x) * 0.25f;
}

} // namespace

bool run_parallax_cards_tests() {
    using namespace tachyon;

    g_failures = 0;

    scene::EvaluatedCompositionState composition;
    composition.composition_id = "main";
    composition.composition_name = "Main";
    composition.width = 1920;
    composition.height = 1080;
    composition.camera.available = true;
    composition.camera.camera.aspect = 1920.0f / 1080.0f;
    composition.camera.camera.transform.compose_trs(
        math::Vector3{40.0f, 0.0f, 0.0f},
        math::Quaternion::identity(),
        math::Vector3::one());

    composition.layers.push_back(make_layer("image_far", "image", 0.0f, 0.0f, 1.0f, 1.0f, 1.0));
    composition.layers.push_back(make_layer("text_near", "text", 0.0f, 0.0f, 1.0f, 1.0f, 1.0));

    const auto draw_list = renderer2d::DrawListBuilder::build(composition);
    check_true(draw_list.commands.size() == 3, "parallax draw list should include clear plus two cards");
    check_true(draw_list.commands[1].textured_quad.has_value(), "far image should project to textured quad");
    check_true(draw_list.commands[2].textured_quad.has_value(), "near text should project to textured quad");

    const auto& far_quad = *draw_list.commands[1].textured_quad;
    const auto& near_quad = *draw_list.commands[2].textured_quad;

    check_true(quad_width(near_quad) > quad_width(far_quad), "near card should appear larger than far card");
    check_true(std::abs(quad_center_x(near_quad) - 960.0f) > std::abs(quad_center_x(far_quad) - 960.0f), "near card should show stronger parallax shift than far card");
    check_true(draw_list.commands[1].z_order < draw_list.commands[2].z_order, "far card should sort before near card");

    return g_failures == 0;
}
