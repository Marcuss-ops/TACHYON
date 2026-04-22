#include "tachyon/renderer2d/evaluated_composition/composition_renderer.h"
#include "tachyon/core/scene/evaluated_state.h"
#include "tachyon/core/spec/scene_spec.h"
#include "tachyon/renderer2d/core/framebuffer.h"
#include "tachyon/renderer2d/raster/rasterizer.h"
#include "tachyon/renderer2d/raster/rasterizer_ops.h"
#include "tachyon/renderer2d/resource/render_context.h"
#include "tachyon/renderer2d/raster/draw_command.h"

#include <cmath>
#include <iostream>
#include <memory>
#include <string>

namespace {

static int g_failures = 0;

static void check_true(bool condition, const std::string& message) {
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
    shape.type = scene::LayerType::Solid;
    shape.enabled = true;
    shape.active = true;
    shape.visible = true;
    shape.opacity = 1.0;
    shape.width = 64;
    shape.height = 32;
    shape.local_transform.position = {8.0f, 8.0f};
    shape.local_transform.scale = {1.0f, 1.0f};
    shape.layer_index = 0;
    state.layers.push_back(shape);

    RenderPlan plan;
    plan.composition.id = "main";
    plan.composition.width = state.width;
    plan.composition.height = state.height;
    plan.composition.layer_count = state.layers.size();

    FrameRenderTask task;
    task.frame_number = 0;
    task.cache_key.value = "renderer-test";

    renderer2d::RenderContext2D render_context;
    const RasterizedFrame2D frame = tachyon::render_evaluated_composition_2d(state, plan, task, render_context);
    check_true(frame.surface != nullptr, "evaluated renderer should produce a surface");
    if (!frame.surface) return false;

    const auto& surface = *frame.surface;
    check_true(surface.width() == 128, "surface width should match composition");
    check_true(surface.height() == 64, "surface height should match composition");
    check_true(surface.get_pixel(16, 16).a > 0, "shape layer should render visible pixels");
    check_true(surface.get_pixel(127, 63).a == 0, "solid layer should not cover the whole surface");

    scene::EvaluatedCompositionState blend_state;
    blend_state.composition_id = "blend";
    blend_state.width = 64;
    blend_state.height = 64;

    scene::EvaluatedLayerState red_layer = shape;
    red_layer.id = "red";
    red_layer.type = scene::LayerType::Solid;
    red_layer.blend_mode = "normal";
    red_layer.fill_color = ColorSpec{255, 0, 0, 255};
    red_layer.width = 32;
    red_layer.height = 32;
    red_layer.local_transform.position = {8.0f, 8.0f};
    red_layer.layer_index = 0;

    scene::EvaluatedLayerState green_layer = red_layer;
    green_layer.id = "green";
    green_layer.blend_mode = "additive";
    green_layer.fill_color = ColorSpec{0, 255, 0, 255};
    green_layer.layer_index = 1;

    blend_state.layers.push_back(red_layer);
    blend_state.layers.push_back(green_layer);

    const RasterizedFrame2D blend_frame = tachyon::render_evaluated_composition_2d(blend_state, plan, task, render_context);
    check_true(blend_frame.surface != nullptr, "blend renderer should produce a surface");
    if (blend_frame.surface) {
        const auto pixel = blend_frame.surface->get_pixel(16, 16);
        check_true(pixel.r > 0, "additive blend should preserve red channel");
        check_true(pixel.g > 0, "additive blend should add green channel");
    }

    const renderer2d::Color multiply_src{1.0f, 0.0f, 0.0f, 0.5f};
    const renderer2d::Color multiply_dst{1.0f, 1.0f, 1.0f, 1.0f};
    const renderer2d::Color multiply_pixel = renderer2d::blend_mode_color(
        multiply_src,
        multiply_dst,
        renderer2d::BlendMode::Multiply);
    check_true(multiply_pixel.r >= 0.99f, "multiply red * white = red");
    check_true(multiply_pixel.g == 0.0f, "multiply red * white (green) = 0");
    check_true(multiply_pixel.b == 0.0f, "multiply red * white (blue) = 0");
    check_true(std::abs(multiply_pixel.a - 0.5f) < 0.01f, "multiply should keep src alpha");

    scene::EvaluatedCompositionState adjustment_state;
    adjustment_state.composition_id = "adjustment";
    adjustment_state.width = 64;
    adjustment_state.height = 64;

    scene::EvaluatedLayerState base_layer = red_layer;
    base_layer.id = "base";
    base_layer.fill_color = ColorSpec{0, 0, 255, 255};
    base_layer.blend_mode = "normal";
    base_layer.layer_index = 0;

    scene::EvaluatedLayerState adjustment_layer = base_layer;
    adjustment_layer.id = "adjustment_layer";
    adjustment_layer.type = scene::LayerType::Solid;
    adjustment_layer.is_adjustment_layer = true;
    adjustment_layer.fill_color = ColorSpec{255, 255, 255, 255};
    adjustment_layer.opacity = 0.75;
    adjustment_layer.layer_index = 1;
    adjustment_layer.effects.clear();
    adjustment_layer.effects.push_back(tachyon::EffectSpec{
        "fill",
        "fill_red",
        true,
        {},
        {{"color", ColorSpec{255, 0, 0, 255}}}
    });

    adjustment_state.layers.push_back(base_layer);
    adjustment_state.layers.push_back(adjustment_layer);

    const RasterizedFrame2D adjustment_frame = tachyon::render_evaluated_composition_2d(adjustment_state, plan, task, render_context);
    check_true(adjustment_frame.surface != nullptr, "adjustment renderer should produce a surface");
    if (adjustment_frame.surface) {
        const auto pixel = adjustment_frame.surface->get_pixel(16, 16);
        check_true(pixel.r > pixel.b, "adjustment layer should bias the composite toward red");
        check_true(pixel.b > 0, "adjustment layer should preserve some of the original image");
    }

    scene::EvaluatedCompositionState timeline_state;
    timeline_state.composition_id = "timeline_main";
    timeline_state.width = 64;
    timeline_state.height = 64;

    scene::EvaluatedLayerState timeline_layer;
    timeline_layer.id = "timeline_solid";
    timeline_layer.type = scene::LayerType::Solid;
    timeline_layer.enabled = true;
    timeline_layer.active = true;
    timeline_layer.visible = true;
    timeline_layer.opacity = 1.0f;
    timeline_layer.width = 64;
    timeline_layer.height = 32;
    timeline_layer.local_transform.position = {8.0f, 8.0f};
    timeline_layer.local_transform.scale = {1.0f, 1.0f};
    timeline_state.layers.push_back(timeline_layer);

    const RasterizedFrame2D timeline_frame = tachyon::render_evaluated_composition_2d(timeline_state, plan, task, render_context);
    check_true(timeline_frame.surface != nullptr, "timeline renderer should produce a surface");
    if (timeline_frame.surface) {
        check_true(timeline_frame.surface->get_pixel(16, 16).a > 0, "timeline renderer should reuse the shared raster path");
    }

    scene::EvaluatedCompositionState matte_state;
    matte_state.composition_id = "matte";
    matte_state.width = 64;
    matte_state.height = 64;

    scene::EvaluatedLayerState matte_layer;
    matte_layer.id = "matte_layer";
    matte_layer.type = scene::LayerType::Solid;
    matte_layer.enabled = true;
    matte_layer.active = true;
    matte_layer.visible = true;
    matte_layer.opacity = 1.0;
    matte_layer.width = 24;
    matte_layer.height = 24;
    matte_layer.fill_color = ColorSpec{255, 255, 255, 255};
    matte_layer.local_transform.position = {16.0f, 16.0f};
    matte_layer.layer_index = 0;

    scene::EvaluatedLayerState matte_target = matte_layer;
    matte_target.id = "matte_target";
    matte_target.width = 64;
    matte_target.height = 64;
    matte_target.fill_color = ColorSpec{255, 0, 0, 255};
    matte_target.track_matte_type = TrackMatteType::Alpha;
    matte_target.track_matte_layer_index = 0;
    matte_target.layer_index = 1;

    matte_state.layers.push_back(matte_layer);
    matte_state.layers.push_back(matte_target);

    const RasterizedFrame2D matte_frame = tachyon::render_evaluated_composition_2d(matte_state, plan, task, render_context);
    check_true(matte_frame.surface != nullptr, "track matte renderer should produce a surface");
    if (matte_frame.surface) {
        const auto& matte_surface = *matte_frame.surface;
        check_true(matte_surface.get_pixel(24, 24).a > 0, "track matte should preserve pixels inside the matte");
        check_true(matte_surface.get_pixel(2, 2).a == 0, "track matte should clear pixels outside the matte");
    }

    scene::EvaluatedCompositionState mask_state;
    mask_state.composition_id = "mask";
    mask_state.width = 64;
    mask_state.height = 64;

    scene::EvaluatedLayerState vector_mask = matte_layer;
    vector_mask.id = "vector_mask";
    vector_mask.type = scene::LayerType::Mask;
    vector_mask.width = 64;
    vector_mask.height = 64;
    vector_mask.layer_index = 0;
    scene::EvaluatedShapePath mask_path;
    mask_path.closed = true;
    mask_path.points.push_back({{16.0f, 16.0f}, {}, {}});
    mask_path.points.push_back({{48.0f, 16.0f}, {}, {}});
    mask_path.points.push_back({{48.0f, 48.0f}, {}, {}});
    mask_path.points.push_back({{16.0f, 48.0f}, {}, {}});
    vector_mask.shape_path = mask_path;

    scene::EvaluatedLayerState masked_layer = matte_target;
    masked_layer.id = "masked_by_vector";
    masked_layer.type = scene::LayerType::Solid;
    masked_layer.fill_color = ColorSpec{0, 255, 0, 255};
    masked_layer.track_matte_type = TrackMatteType::None;
    masked_layer.track_matte_layer_index.reset();
    masked_layer.layer_index = 1;

    mask_state.layers.push_back(vector_mask);
    mask_state.layers.push_back(masked_layer);

    const RasterizedFrame2D mask_frame = tachyon::render_evaluated_composition_2d(mask_state, plan, task, render_context);
    check_true(mask_frame.surface != nullptr, "mask renderer should produce a surface");
    if (mask_frame.surface) {
        const auto& mask_surface = *mask_frame.surface;
        check_true(mask_surface.get_pixel(24, 24).a > 0, "vector mask should preserve pixels inside the shape");
        check_true(mask_surface.get_pixel(8, 8).a == 0, "vector mask should clear pixels outside the shape");
    }

    renderer2d::RenderContext2D precomp_context;
    precomp_context.policy = make_quality_policy("draft");
    check_true(precomp_context.precomp_cache != nullptr, "precomp cache should exist");

    scene::EvaluatedCompositionState child_state;
    child_state.composition_id = "child";
    child_state.width = 32;
    child_state.height = 32;

    scene::EvaluatedLayerState child_layer = base_layer;
    child_layer.id = "child_solid";
    child_layer.width = 32;
    child_layer.height = 32;
    child_layer.local_transform.position = {16.0f, 16.0f};
    child_layer.layer_index = 0;
    child_state.layers.push_back(child_layer);

    scene::EvaluatedCompositionState parent_state;
    parent_state.composition_id = "parent";
    parent_state.width = 64;
    parent_state.height = 64;

    scene::EvaluatedLayerState precomp_layer = base_layer;
    precomp_layer.id = "precomp";
    precomp_layer.type = scene::LayerType::Precomp;
    precomp_layer.precomp_id = std::string{"child"};
    precomp_layer.nested_composition = std::make_unique<scene::EvaluatedCompositionState>(child_state);
    precomp_layer.width = child_state.width;
    precomp_layer.height = child_state.height;
    precomp_layer.local_transform.position = {32.0f, 32.0f};
    precomp_layer.layer_index = 0;
    parent_state.layers.push_back(precomp_layer);

    const RasterizedFrame2D precomp_first = tachyon::render_evaluated_composition_2d(parent_state, plan, task, precomp_context);
    const std::size_t cache_entries_after_first = precomp_context.precomp_cache->entry_count();
    const RasterizedFrame2D precomp_second = tachyon::render_evaluated_composition_2d(parent_state, plan, task, precomp_context);
    check_true(precomp_first.surface != nullptr, "precomp renderer should produce a surface");
    check_true(precomp_second.surface != nullptr, "precomp renderer should render a second frame");
    check_true(cache_entries_after_first > 0, "precomp cache should store the nested render after first frame");
    check_true(precomp_context.precomp_cache->entry_count() == cache_entries_after_first, "second frame should reuse the existing precomp cache entry");

    return g_failures == 0;
}
