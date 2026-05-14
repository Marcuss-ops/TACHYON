#include "tachyon/runtime/execution/planning/render_plan.h"
#include "tachyon/runtime/cache/cache_key_builder.h"

#include <algorithm>
#include <cmath>

namespace tachyon {
namespace {

const CompositionSpec* find_composition_internal(const SceneSpec& scene, const std::string& composition_id) {
    for (const auto& comp : scene.compositions) {
        if (comp.id == composition_id) return &comp;
    }
    return nullptr;
}

std::size_t count_layers_with_type_internal(const CompositionSpec& composition, LayerType type) {
    std::size_t count = 0;
    for (const auto& layer : composition.layers) {
        if (layer.identity.type == type) count++;
    }
    return count;
}

std::size_t count_layers_with_track_matte_internal(const CompositionSpec& composition) {
    std::size_t count = 0;
    for (const auto& layer : composition.layers) {
        if (layer.track_matte_type != TrackMatteType::None) count++;
    }
    return count;
}

CompositionSummary make_composition_summary_internal(const CompositionSpec& comp) {
    CompositionSummary summary;
    summary.id = comp.id;
    summary.name = comp.name;
    summary.width = static_cast<std::int64_t>(comp.width);
    summary.height = static_cast<std::int64_t>(comp.height);
    summary.duration = comp.duration;
    summary.frame_rate = comp.frame_rate;
    summary.background = comp.background;
    summary.layer_count = comp.layers.size();
    summary.solid_layer_count = count_layers_with_type_internal(comp, LayerType::Solid);
    summary.shape_layer_count = count_layers_with_type_internal(comp, LayerType::Shape);
    summary.mask_layer_count = count_layers_with_type_internal(comp, LayerType::Mask);
    summary.image_layer_count = count_layers_with_type_internal(comp, LayerType::Image);
    summary.text_layer_count = count_layers_with_type_internal(comp, LayerType::Text);
    summary.precomp_layer_count = count_layers_with_type_internal(comp, LayerType::Precomp);
    summary.track_matte_layer_count = count_layers_with_track_matte_internal(comp);
    return summary;
}

} // namespace

ResolutionResult<RenderPlan> build_render_plan(const SceneSpec& scene, const RenderJob& job) {
    ResolutionResult<RenderPlan> result;
    const auto* comp_spec = find_composition_internal(scene, job.composition_target);
    if (!comp_spec) {
        result.diagnostics.add_error("composition_not_found", "Composition not found: " + job.composition_target);
        return result;
    }

    RenderPlan plan;
    plan.job_id = job.job_id;
    plan.scene_ref = job.scene_ref;
    plan.composition_target = job.composition_target;
    plan.composition = make_composition_summary_internal(*comp_spec);
    
    // Logic for frame range
    if (job.frame_range.start == 0 && job.frame_range.end == 0) {
        plan.frame_range.start = 0;
        plan.frame_range.end = static_cast<std::int64_t>(std::ceil(comp_spec->duration * comp_spec->frame_rate.value()));
    } else {
        plan.frame_range = job.frame_range;
    }

    plan.quality_tier = job.quality_tier;
    plan.motion_blur_enabled = job.motion_blur_enabled;
    plan.motion_blur_samples = job.motion_blur_samples;
    plan.output = job.output;
    plan.compositing_alpha_mode = job.compositing_alpha_mode;
    plan.working_space = job.working_space;
    plan.seed_policy_mode = job.seed_policy_mode;
    plan.compatibility_mode = job.compatibility_mode;
    plan.proxy_enabled = job.proxy_enabled;
    plan.variables = job.variables;
    plan.string_variables = job.string_variables;
    plan.layer_overrides = job.layer_overrides;
    
    plan.scene_spec = &scene;
    plan.scene_hash = 0; 
    
    result.value = std::move(plan);
    return result;
}

ResolutionResult<RenderExecutionPlan> build_render_execution_plan(const RenderPlan& plan, std::size_t assets_count) {
    ResolutionResult<RenderExecutionPlan> result;
    RenderExecutionPlan exec;
    exec.render_plan = plan;
    exec.resolved_asset_count = assets_count;

    if (!plan.scene_spec) {
        result.diagnostics.add_error("missing_scene", "Missing scene spec in render plan");
        return result;
    }

    const double fps = plan.composition.frame_rate.value();
    const std::int64_t start = plan.frame_range.start;
    const std::int64_t end = plan.frame_range.end;

    for (std::int64_t i = start; i < end; ++i) {
        FrameRenderTask task;
        task.frame_number = i;
        task.time_seconds = static_cast<double>(i) / fps;
        
        CacheKeyBuilder builder;
        builder.add_u64(plan.scene_hash);
        builder.add_u64(static_cast<std::uint64_t>(i));
        task.cache_key = CacheKey(builder.finish(), "");
        
        exec.frame_tasks.push_back(std::move(task));
    }

    RenderStep step;
    step.id = "rasterize_main";
    step.kind = RenderStepKind::Rasterize2DFrame;
    step.label = "Main Composition Rasterization";
    exec.steps.push_back(std::move(step));

    result.value = std::move(exec);
    return result;
}

FrameCacheKey build_frame_cache_key(const RenderPlan& plan, std::int64_t frame_number) {
    CacheKeyBuilder builder;
    builder.add_u64(plan.scene_hash);
    builder.add_u64(static_cast<std::uint64_t>(frame_number));
    return FrameCacheKey(builder.finish(), "");
}

bool frame_cache_entry_matches(const FrameCacheEntry& entry, const FrameCacheKey& key) {
    return entry.key.hash == key.hash;
}

std::string render_step_kind_string(RenderStepKind kind) {
    switch (kind) {
        case RenderStepKind::Unknown: return "unknown";
        case RenderStepKind::Precomp: return "precomp";
        case RenderStepKind::Layer: return "layer";
        case RenderStepKind::Effect: return "effect";
        case RenderStepKind::Rasterize2DFrame: return "rasterize_2d";
        case RenderStepKind::Output: return "output";
    }
    return "unknown";
}

} // namespace tachyon
