#include "tachyon/renderer2d/evaluated_composition/composition_renderer.h"
#include "tachyon/render/render_intent.h"
#include "tachyon/renderer2d/effects/effect_registry.h"
#include "tachyon/transition_registry.h"
#include "tachyon/renderer2d/effects/core/transitions/transition_fast_paths.h"
#include "tachyon/core/scene/state/evaluated_state.h"
#include "tachyon/core/spec/schema/objects/scene_spec.h"
#include "tachyon/renderer2d/core/framebuffer.h"
#include "tachyon/renderer2d/raster/rasterizer.h"
#include "tachyon/renderer2d/raster/rasterizer_ops.h"
#include "tachyon/runtime/resource/render_context.h"
#include "tachyon/renderer2d/resource/precomp_cache.h"
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
    using namespace tachyon::render;

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

    RenderContext render_context;
    renderer2d::EffectRegistry effect_reg;
    TransitionRegistry transition_reg;
    register_builtin_effects(effect_reg, transition_reg);

    RenderIntent intent;
    const RasterizedFrame2D frame = tachyon::render_evaluated_composition_2d(state, intent, plan, task, render_context, effect_reg);
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
    red_layer.text.fill_color = ColorSpec{255, 0, 0, 255};
    red_layer.width = 32;
    red_layer.height = 32;
    red_layer.local_transform.position = {8.0f, 8.0f};
    red_layer.layer_index = 0;

    scene::EvaluatedLayerState green_layer = red_layer;
    green_layer.id = "green";
    green_layer.blend_mode = "additive";
    green_layer.text.fill_color = ColorSpec{0, 255, 0, 255};
    green_layer.layer_index = 1;

    blend_state.layers.push_back(red_layer);
    blend_state.layers.push_back(green_layer);

    RenderIntent intent_blend;
    const RasterizedFrame2D blend_frame = tachyon::render_evaluated_composition_2d(blend_state, intent_blend, plan, task, render_context, effect_reg);
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
    check_true(multiply_pixel.r >= 0.99f, "multiply red * white should keep red");
    check_true(std::abs(multiply_pixel.g - 0.5f) < 0.001f, "multiply red * white (green) should keep destination contribution");
    check_true(std::abs(multiply_pixel.b - 0.5f) < 0.001f, "multiply red * white (blue) should keep destination contribution");
    check_true(std::abs(multiply_pixel.a - 1.0f) < 0.001f, "multiply alpha should be 1.0 (src_a + dst_a - src_a * dst_a)");

    scene::EvaluatedCompositionState adjustment_state;
    adjustment_state.composition_id = "adjustment";
    adjustment_state.width = 64;
    adjustment_state.height = 64;

    scene::EvaluatedLayerState base_layer = red_layer;
    base_layer.id = "base";
    base_layer.text.fill_color = ColorSpec{0, 0, 255, 255};
    base_layer.blend_mode = "normal";
    base_layer.layer_index = 0;

    scene::EvaluatedLayerState adjustment_layer = base_layer;
    adjustment_layer.id = "adjustment_layer";
    adjustment_layer.type = scene::LayerType::Solid;
    adjustment_layer.is_adjustment_layer = true;
    adjustment_layer.text.fill_color = ColorSpec{255, 255, 255, 255};
    adjustment_layer.opacity = 0.75;
    adjustment_layer.layer_index = 1;
    adjustment_layer.effects.clear();
    EffectSpec fill_effect;
    fill_effect.type = "tachyon.effect.color.fill";
    fill_effect.enabled = true;
    fill_effect.params["color"] = ParameterValue{ColorSpec{255, 0, 0, 255}};
    adjustment_layer.effects.push_back(fill_effect.evaluate(0.0));

    adjustment_state.layers.push_back(base_layer);
    adjustment_state.layers.push_back(adjustment_layer);

    RenderIntent intent_adj;
    const RasterizedFrame2D adjustment_frame = tachyon::render_evaluated_composition_2d(adjustment_state, intent_adj, plan, task, render_context, effect_reg);
    check_true(adjustment_frame.surface != nullptr, "adjustment renderer should produce a surface");
    if (adjustment_frame.surface) {
        const auto pixel = adjustment_frame.surface->get_pixel(16, 16);
        check_true(pixel.r > pixel.b, "adjustment layer should bias the composite toward red");
        check_true(pixel.b > 0, "adjustment layer should preserve some of the original image");
    }

    scene::EvaluatedCompositionState transition_state;
    transition_state.composition_id = "transition";
    transition_state.width = 64;
    transition_state.height = 64;

    scene::EvaluatedLayerState transition_layer = base_layer;
    transition_layer.id = "transition_layer";
    transition_layer.width = 64;
    transition_layer.height = 64;
    transition_layer.text.fill_color = ColorSpec{255, 0, 0, 255};
    transition_layer.transition_in.transition_id = "tachyon.transition.crossfade";
    transition_layer.transition_in.duration = 1.0;
    transition_layer.in_time = 0.0;
    transition_layer.layer_index = 0;

    transition_state.layers.push_back(transition_layer);

    RenderPlan transition_plan = plan;
    transition_plan.composition.id = transition_state.composition_id;
    transition_plan.composition.width = transition_state.width;
    transition_plan.composition.height = transition_state.height;
    transition_plan.composition.layer_count = transition_state.layers.size();

    RenderContext transition_context = render_context;
    transition_context.policy = make_quality_policy("draft");
    transition_context.transition_registry = &transition_reg;

    RenderIntent intent_transition;
    FrameRenderTask transition_task = task;
    transition_task.time_seconds = 0.0;
    const RasterizedFrame2D transition_start = tachyon::render_evaluated_composition_2d(
        transition_state,
        intent_transition,
        transition_plan,
        transition_task,
        transition_context,
        effect_reg);
    transition_task.time_seconds = 0.5;
    const RasterizedFrame2D transition_mid = tachyon::render_evaluated_composition_2d(
        transition_state,
        intent_transition,
        transition_plan,
        transition_task,
        transition_context,
        effect_reg);
    check_true(transition_start.surface != nullptr, "transition renderer should produce a start frame");
    check_true(transition_mid.surface != nullptr, "transition renderer should produce a mid frame");
    if (transition_start.surface && transition_mid.surface) {
        const auto start_pixel = transition_start.surface->get_pixel(16, 16);
        const auto mid_pixel = transition_mid.surface->get_pixel(16, 16);
        check_true(
            std::abs(mid_pixel.r - start_pixel.r) > 0.01f || std::abs(mid_pixel.a - start_pixel.a) > 0.01f,
            "layer transitions should animate even when draft quality disables heavier effects");
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

    RenderIntent intent_timeline;
    const RasterizedFrame2D timeline_frame = tachyon::render_evaluated_composition_2d(timeline_state, intent_timeline, plan, task, render_context, effect_reg);
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
    matte_layer.text.fill_color = ColorSpec{255, 255, 255, 255};
    matte_layer.local_transform.position = {16.0f, 16.0f};
    matte_layer.layer_index = 0;

    scene::EvaluatedLayerState matte_target = matte_layer;
    matte_target.id = "matte_target";
    matte_target.width = 64;
    matte_target.height = 64;
    matte_target.text.fill_color = ColorSpec{255, 0, 0, 255};
    matte_target.track_matte_type = TrackMatteType::Alpha;
    matte_target.track_matte_layer_index = 0;
    matte_target.layer_index = 1;

    matte_state.layers.push_back(matte_layer);
    matte_state.layers.push_back(matte_target);

    RenderIntent intent_matte;
    const RasterizedFrame2D matte_frame = tachyon::render_evaluated_composition_2d(matte_state, intent_matte, plan, task, render_context, effect_reg);
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

    scene::EvaluatedLayerState masked_layer = matte_target;
    masked_layer.id = "masked_by_vector";
    masked_layer.type = scene::LayerType::Solid;
    masked_layer.text.fill_color = ColorSpec{0, 255, 0, 255};
    masked_layer.track_matte_type = TrackMatteType::None;
    masked_layer.track_matte_layer_index.reset();
    masked_layer.layer_index = 1;

    mask_state.layers.push_back(vector_mask);
    mask_state.layers.push_back(masked_layer);

    RenderIntent intent_mask;
    const RasterizedFrame2D mask_frame = tachyon::render_evaluated_composition_2d(mask_state, intent_mask, plan, task, render_context, effect_reg);
    check_true(mask_frame.surface != nullptr, "mask renderer should produce a surface");
    if (mask_frame.surface) {
        const auto& mask_surface = *mask_frame.surface;
        check_true(mask_surface.get_pixel(24, 24).a > 0, "vector mask should preserve pixels inside the shape");
        check_true(mask_surface.get_pixel(8, 8).a == 0, "vector mask should clear pixels outside the shape");
    }

    RenderContext precomp_context;
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
    precomp_layer.nested_composition = std::make_shared<scene::EvaluatedCompositionState>(child_state);
    precomp_layer.width = child_state.width;
    precomp_layer.height = child_state.height;
    precomp_layer.local_transform.position = {32.0f, 32.0f};
    precomp_layer.layer_index = 0;
    parent_state.layers.push_back(precomp_layer);
    {
        RenderIntent p_intent;
        const RasterizedFrame2D precomp_first = tachyon::render_evaluated_composition_2d(parent_state, p_intent, plan, task, precomp_context, effect_reg);
        const std::size_t cache_entries_after_first = precomp_context.precomp_cache->entry_count();
        const RasterizedFrame2D precomp_second = tachyon::render_evaluated_composition_2d(parent_state, p_intent, plan, task, precomp_context, effect_reg);
        check_true(precomp_first.surface != nullptr, "precomp renderer should produce a surface");
        check_true(precomp_second.surface != nullptr, "precomp renderer should render a second frame");
        check_true(cache_entries_after_first > 0, "precomp cache should store the nested render after first frame");
        check_true(precomp_context.precomp_cache->entry_count() == cache_entries_after_first, "second frame should reuse the existing precomp cache entry");
    }

    return g_failures == 0;
}
