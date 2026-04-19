#include "tachyon/renderer2d/evaluated_composition_renderer.h"

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

} // namespace

bool run_evaluated_composition_renderer_tests() {
    using namespace tachyon;

    scene::EvaluatedCompositionState state;
    state.composition_id = "main";
    state.width = 128;
    state.height = 64;

    scene::EvaluatedLayerState shape;
    shape.id = "shape";
    shape.type = "shape";
    shape.visible = true;
    shape.opacity = 1.0;
    shape.position = {8.0f, 8.0f};
    shape.scale = {1.0f, 1.0f};
    shape.layer_index = 0;
    state.layers.push_back(shape);

    scene::EvaluatedLayerState text;
    text.id = "text";
    text.type = "text";
    text.name = "TACHYON";
    text.visible = true;
    text.opacity = 1.0;
    text.position = {48.0f, 10.0f};
    text.scale = {1.0f, 1.0f};
    text.layer_index = 1;
    state.layers.push_back(text);

    RenderPlan plan;
    plan.composition.id = "main";
    plan.composition.width = state.width;
    plan.composition.height = state.height;
    plan.composition.layer_count = state.layers.size();

    FrameRenderTask task;
    task.frame_number = 0;
    task.cache_key.value = "renderer-test";

    const RasterizedFrame2D frame = tachyon::render_evaluated_composition_2d(state, plan, task);
    check_true(frame.surface.has_value(), "evaluated renderer should produce a surface");
    if (!frame.surface.has_value()) {
        return false;
    }

    const auto& surface = *frame.surface;
    check_true(surface.width() == 128, "surface width should match composition");
    check_true(surface.height() == 64, "surface height should match composition");
    check_true(surface.get_pixel(16, 16).a > 0, "shape layer should render visible pixels");

    bool text_visible = false;
    for (std::uint32_t y = 0; y < surface.height(); ++y) {
        for (std::uint32_t x = 48; x < surface.width(); ++x) {
            if (surface.get_pixel(x, y).a > 0) {
                text_visible = true;
                break;
            }
        }
        if (text_visible) {
            break;
        }
    }
    check_true(text_visible, "text layer should render visible pixels");

    return g_failures == 0;
}
