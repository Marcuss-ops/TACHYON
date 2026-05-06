#include "tachyon/runtime/execution/planning/render_plan.h"
#include "tachyon/runtime/execution/frames/frame_executor.h"
#include "tachyon/runtime/cache/cache_key_builder.h"
#include "tachyon/text/fonts/management/font_manifest.h"

#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <span>
#include <string_view>

namespace tachyon {
// Forward declaration for function defined in render_plan_hash.cpp
std::uint64_t hash_scene_content(const SceneSpec& scene);

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

std::size_t count_layers_with_type(const CompositionSpec& composition, LayerType type) {
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

std::uint64_t hash_font_content(const SceneSpec& scene) {
    if (!scene.font_manifest) {
        return 0;
    }

    CacheKeyBuilder builder;
    builder.add_u64(static_cast<std::uint64_t>(scene.font_manifest->fonts.size()));
    for (const auto& entry : scene.font_manifest->fonts) {
        builder.add_string(entry.id);
        builder.add_string(entry.family);
        builder.add_string(entry.src.string());
        builder.add_u64(static_cast<std::uint64_t>(entry.weight));
        builder.add_u32(static_cast<std::uint32_t>(entry.style));
        builder.add_u32(static_cast<std::uint32_t>(entry.stretch));
        builder.add_string(entry.format);
        builder.add_bool(entry.is_fallback);

        if (std::filesystem::exists(entry.src) && std::filesystem::is_regular_file(entry.src)) {
            std::ifstream file(entry.src, std::ios::binary);
            if (file) {
                std::array<std::byte, 4096> buffer{};
                while (file) {
                    file.read(reinterpret_cast<char*>(buffer.data()), static_cast<std::streamsize>(buffer.size()));
                    const std::streamsize read = file.gcount();
                    if (read > 0) {
                        builder.add_bytes(std::span<const std::byte>(buffer.data(), static_cast<std::size_t>(read)));
                    }
                }
            }
        }
    }

    return builder.finish();
}

CompositionSummary make_summary(const CompositionSpec& composition) {
    CompositionSummary summary;
    summary.id = composition.id;
    summary.name = composition.name;
    summary.width = composition.width;
    summary.height = composition.height;
    summary.duration = composition.duration;
    summary.frame_rate = composition.frame_rate;
    summary.background = composition.background; // Copy optional<BackgroundSpec>
    summary.layer_count = composition.layers.size();
    summary.solid_layer_count = count_layers_with_type(composition, LayerType::Solid);
    summary.shape_layer_count = count_layers_with_type(composition, LayerType::Shape);
    summary.mask_layer_count = count_layers_with_type(composition, LayerType::Mask);
    summary.image_layer_count = count_layers_with_type(composition, LayerType::Image);
    summary.text_layer_count = count_layers_with_type(composition, LayerType::Text);
    summary.precomp_layer_count = count_layers_with_type(composition, LayerType::Precomp);
    summary.track_matte_layer_count = count_layers_with_track_matte(composition);
    return summary;
}

} // namespace

ResolutionResult<RenderPlan> build_render_plan(const SceneSpec& scene, const RenderJob& job) {
    ResolutionResult<RenderPlan> result;
    RenderJob resolved_job = job;
    apply_output_preset(resolved_job.output.profile);

    const CompositionSpec* composition = find_composition(scene, resolved_job.composition_target);
    if (composition == nullptr) {
        result.diagnostics.add_error(
            "plan.composition_missing",
            "composition_target does not match any composition in the scene",
            "composition_target");
        return result;
    }

    RenderPlan plan;
    plan.job_id = resolved_job.job_id;
    plan.scene_ref = resolved_job.scene_ref;
    plan.composition_target = resolved_job.composition_target;
    plan.composition = make_summary(*composition);
    plan.frame_range = resolved_job.frame_range;
    plan.output = resolved_job.output;
    plan.quality_tier = resolved_job.quality_tier;
    plan.quality_policy = make_quality_policy(plan.quality_tier);
    plan.compositing_alpha_mode = resolved_job.compositing_alpha_mode;
    plan.working_space = resolved_job.working_space;
    plan.motion_blur_enabled = resolved_job.motion_blur_enabled;
    plan.motion_blur_samples = resolved_job.motion_blur_samples;
    if (plan.motion_blur_enabled && plan.motion_blur_samples <= 0) {
        plan.motion_blur_samples = 8;
    }
    plan.motion_blur_shutter_angle = resolved_job.motion_blur_shutter_angle;
    plan.motion_blur_curve = resolved_job.motion_blur_curve;
    plan.seed_policy_mode = resolved_job.seed_policy_mode;
    plan.compatibility_mode = resolved_job.compatibility_mode;
    plan.scene_spec = &scene;
    plan.variables = resolved_job.variables;
    plan.string_variables = resolved_job.string_variables;
    plan.layer_overrides = resolved_job.layer_overrides;

    // Canonical fields correctly populated
    plan.scene_hash = hash_scene_content(scene);
    plan.font_content_hash = hash_font_content(scene);
    plan.contract_version = 1;
    plan.proxy_enabled = resolved_job.proxy_enabled;

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

    const double fps = plan.composition.frame_rate.numerator > 0 
        ? static_cast<double>(plan.composition.frame_rate.numerator) / static_cast<double>(plan.composition.frame_rate.denominator)
        : 60.0;

    for (std::int64_t frame = plan.frame_range.start; frame <= plan.frame_range.end; ++frame) {
        FrameRenderTask task;
        task.frame_number = frame;
        task.time_seconds = static_cast<double>(frame) / fps;
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

} // namespace tachyon
