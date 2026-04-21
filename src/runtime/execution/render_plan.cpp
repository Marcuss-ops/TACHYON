#include "tachyon/runtime/execution/render_plan.h"
#include "tachyon/runtime/cache/cache_key_builder.h"

#include <algorithm>
#include <string_view>

namespace tachyon {
namespace {

const CompositionSpec* find_composition(const SceneSpec& scene, const std::string& composition_id) {
    const auto it = std::find_if(scene.compositions.begin(), scene.compositions.end(), [&](const CompositionSpec& composition) {
        return composition.id == composition_id;
    });
    if (it == scene.compositions.end()) {
        return nullptr;
    }
    return &(*it);
}

std::size_t count_layers_with_type(const CompositionSpec& composition, const std::string& type) {
    return static_cast<std::size_t>(std::count_if(
        composition.layers.begin(),
        composition.layers.end(),
        [&](const LayerSpec& layer) { return layer.type == type; }));
}

std::size_t count_layers_with_track_matte(const CompositionSpec& composition) {
    return static_cast<std::size_t>(std::count_if(
        composition.layers.begin(),
        composition.layers.end(),
        [&](const LayerSpec& layer) { return layer.track_matte_type != TrackMatteType::None; }));
}

CompositionSummary make_summary(const CompositionSpec& composition) {
    CompositionSummary summary;
    summary.id = composition.id;
    summary.name = composition.name;
    summary.width = composition.width;
    summary.height = composition.height;
    summary.duration = composition.duration;
    summary.frame_rate = composition.frame_rate;
    summary.background = composition.background;
    summary.layer_count = composition.layers.size();
    summary.solid_layer_count = count_layers_with_type(composition, "solid");
    summary.shape_layer_count = count_layers_with_type(composition, "shape");
    summary.mask_layer_count = count_layers_with_type(composition, "mask");
    summary.image_layer_count = count_layers_with_type(composition, "image");
    summary.text_layer_count = count_layers_with_type(composition, "text");
    summary.precomp_layer_count = static_cast<std::size_t>(std::count_if(
        composition.layers.begin(),
        composition.layers.end(),
        [](const LayerSpec& layer) { return layer.precomp_id.has_value(); }));
    summary.track_matte_layer_count = count_layers_with_track_matte(composition);
    return summary;
}

} // namespace

ResolutionResult<RenderPlan> build_render_plan(const SceneSpec& scene, const RenderJob& job) {
    ResolutionResult<RenderPlan> result;

    const CompositionSpec* composition = find_composition(scene, job.composition_target);
    if (composition == nullptr) {
        result.diagnostics.add_error(
            "plan.composition_missing",
            "composition_target does not match any composition in the scene",
            "composition_target");
        return result;
    }

    RenderPlan plan;
    plan.job_id = job.job_id;
    plan.scene_ref = job.scene_ref;
    plan.composition_target = job.composition_target;
    plan.composition = make_summary(*composition);
    plan.frame_range = job.frame_range;
    plan.output = job.output;
    plan.quality_tier = job.quality_tier;
    plan.quality_policy = make_quality_policy(plan.quality_tier);
    plan.compositing_alpha_mode = job.compositing_alpha_mode;
    plan.working_space = job.working_space;
    plan.motion_blur_enabled = job.motion_blur_enabled;
    plan.motion_blur_samples = job.motion_blur_samples;
    if (plan.motion_blur_enabled && plan.motion_blur_samples <= 0) {
        plan.motion_blur_samples = 8;
    }
    plan.motion_blur_shutter_angle = job.motion_blur_shutter_angle;
    plan.motion_blur_curve = job.motion_blur_curve;
    plan.seed_policy_mode = job.seed_policy_mode;
    plan.compatibility_mode = job.compatibility_mode;
    plan.scene_spec = &scene;
    plan.variables = job.variables;
    plan.string_variables = job.string_variables;
    plan.layer_overrides = job.layer_overrides;

    result.value = std::move(plan);
    return result;
}

std::string render_step_kind_string(RenderStepKind kind) {
    switch (kind) {
        case RenderStepKind::Precomp: return "precomp";
        case RenderStepKind::Layer:   return "layer";
        case RenderStepKind::Effect:  return "effect";
        case RenderStepKind::Rasterize2DFrame: return "rasterize-2d-frame";
        case RenderStepKind::Output:  return "output";
        default:                      return "unknown";
    }
}

ResolutionResult<RenderExecutionPlan> build_render_execution_plan(const RenderPlan& plan, std::size_t assets_count) {
    ResolutionResult<RenderExecutionPlan> result;
    RenderExecutionPlan execution_plan;
    execution_plan.render_plan = plan;
    execution_plan.resolved_asset_count = assets_count;

    for (std::int64_t frame = plan.frame_range.start; frame <= plan.frame_range.end; ++frame) {
        FrameRenderTask task;
        task.frame_number = frame;
        task.cache_key = build_frame_cache_key(plan, frame);
        task.cacheable = true;
        execution_plan.frame_tasks.push_back(std::move(task));
    }

    // High-level execution steps
    auto add_step = [&](std::string id, RenderStepKind kind, std::string label, std::vector<std::string> dependencies = {}) {
        RenderStep step;
        step.id = std::move(id);
        step.kind = kind;
        step.label = std::move(label);
        step.dependencies = std::move(dependencies);
        execution_plan.steps.push_back(std::move(step));
    };

    if (assets_count > 0) {
        add_step("assets", RenderStepKind::Unknown, "Resolving Assets");
    }

    add_step("frame-setup", RenderStepKind::Unknown, "Frame Context Setup", assets_count > 0 ? std::vector<std::string>{"assets"} : std::vector<std::string>{});
    add_step("cache-key", RenderStepKind::Unknown, "Cache Key Preparation", {"frame-setup"});
    add_step("roi", RenderStepKind::Unknown, "ROI Preparation", {"cache-key"});
    add_step("graph", RenderStepKind::Unknown, "Dependency Graph Resolve", {"roi"});
    add_step("raster", RenderStepKind::Rasterize2DFrame, "Frame Rasterization Loop", {"graph", "roi"});
    add_step("output", RenderStepKind::Output, "Final Pipeline Sink Flush", {"raster"});

    result.value = std::move(execution_plan);
    return result;
}

FrameCacheKey build_frame_cache_key(const RenderPlan& plan, std::int64_t frame_number) {
    CacheKeyBuilder builder;
    builder.add_string(plan.job_id);
    builder.add_string(plan.scene_ref);
    builder.add_string(plan.composition_target);
    builder.add_string(plan.quality_tier);
    builder.add_string(plan.compositing_alpha_mode);
    builder.add_string(plan.working_space);
    builder.add_string(plan.motion_blur_curve);
    builder.add_string(plan.seed_policy_mode);
    builder.add_string(plan.compatibility_mode);
    builder.add_u64(static_cast<std::uint64_t>(plan.composition.width));
    builder.add_u64(static_cast<std::uint64_t>(plan.composition.height));
    builder.add_u64(static_cast<std::uint64_t>(plan.frame_range.start));
    builder.add_u64(static_cast<std::uint64_t>(plan.frame_range.end));
    builder.add_u64(static_cast<std::uint64_t>(frame_number));
    builder.add_u64(static_cast<std::uint64_t>(plan.motion_blur_enabled ? 1U : 0U));
    builder.add_u64(static_cast<std::uint64_t>(plan.motion_blur_samples));
    builder.add_u64(static_cast<std::uint64_t>(plan.motion_blur_shutter_angle * 1000.0));
    builder.add_u64(static_cast<std::uint64_t>(plan.composition.layer_count));
    builder.add_u64(static_cast<std::uint64_t>(plan.composition.text_layer_count));
    builder.add_u64(static_cast<std::uint64_t>(plan.composition.precomp_layer_count));

    FrameCacheKey key;
    key.hash = builder.finish();
    key.value =
        plan.job_id + ":" +
        plan.scene_ref + ":" +
        plan.composition_target + ":" +
        plan.quality_tier + ":" +
        plan.compositing_alpha_mode + ":" +
        plan.working_space + ":" +
        plan.motion_blur_curve + ":" +
        plan.seed_policy_mode + ":" +
        plan.compatibility_mode + ":" +
        std::to_string(plan.composition.width) + "x" +
        std::to_string(plan.composition.height) + ":" +
        std::to_string(plan.frame_range.start) + "-" +
        std::to_string(plan.frame_range.end) + ":f" +
        std::to_string(frame_number) + ":mb" +
        std::to_string(plan.motion_blur_enabled ? 1 : 0) + ":" +
        std::to_string(plan.motion_blur_samples) + ":" +
        std::to_string(static_cast<std::int64_t>(plan.motion_blur_shutter_angle * 1000.0));
    return key;
}

bool frame_cache_entry_matches(const FrameCacheEntry& entry, const FrameCacheKey& key) {
    return entry.key.hash == key.hash && entry.key.value == key.value;
}

} // namespace tachyon
