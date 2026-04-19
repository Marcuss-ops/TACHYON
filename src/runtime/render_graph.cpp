#include "tachyon/runtime/render_graph.h"

#include <sstream>
#include <utility>

namespace tachyon {
namespace {

void append_part(std::ostringstream& stream, const char* key, const std::string& value) {
    stream << key << '=' << value << ';';
}

void append_part(std::ostringstream& stream, const char* key, std::int64_t value) {
    stream << key << '=' << value << ';';
}

void append_part(std::ostringstream& stream, const char* key, double value) {
    stream << key << '=' << value << ';';
}

void append_optional(std::ostringstream& stream, const char* key, const std::optional<std::string>& value) {
    if (value.has_value()) {
        append_part(stream, key, *value);
    }
}

void append_optional(std::ostringstream& stream, const char* key, const std::optional<std::int64_t>& value) {
    if (value.has_value()) {
        append_part(stream, key, *value);
    }
}

void append_optional(std::ostringstream& stream, const char* key, const std::optional<double>& value) {
    if (value.has_value()) {
        append_part(stream, key, *value);
    }
}

FrameCacheKey make_key(const RenderPlan& plan, std::int64_t frame_number) {
    std::ostringstream stream;
    append_part(stream, "composition", plan.composition.id);
    append_part(stream, "target", plan.composition_target);
    append_part(stream, "frame", frame_number);
    append_part(stream, "width", plan.composition.width);
    append_part(stream, "height", plan.composition.height);
    append_part(stream, "fps_num", plan.composition.frame_rate.numerator);
    append_part(stream, "fps_den", plan.composition.frame_rate.denominator);
    append_part(stream, "duration", plan.composition.duration);
    append_part(stream, "layer_count", static_cast<std::int64_t>(plan.composition.layer_count));
    append_optional(stream, "background", plan.composition.background);
    append_part(stream, "seed", plan.seed_policy_mode);
    append_part(stream, "compat", plan.compatibility_mode);
    append_part(stream, "profile_name", plan.output.profile.name);
    append_part(stream, "profile_class", plan.output.profile.class_name);
    append_part(stream, "container", plan.output.profile.container);
    append_part(stream, "video_codec", plan.output.profile.video.codec);
    append_part(stream, "pixel_format", plan.output.profile.video.pixel_format);
    append_part(stream, "rate_control", plan.output.profile.video.rate_control_mode);
    append_optional(stream, "crf", plan.output.profile.video.crf);
    append_part(stream, "color_transfer", plan.output.profile.color.transfer);
    append_part(stream, "color_range", plan.output.profile.color.range);

    FrameCacheKey key;
    key.value = stream.str();
    return key;
}

RenderGraphStep make_step(std::string id, RenderStepKind kind, std::string label) {
    RenderGraphStep step;
    step.id = std::move(id);
    step.kind = kind;
    step.label = std::move(label);
    return step;
}

} // namespace

ResolutionResult<RenderExecutionPlan> build_render_execution_plan(const RenderPlan& plan) {
    ResolutionResult<RenderExecutionPlan> result;

    if (plan.frame_range.end < plan.frame_range.start) {
        result.diagnostics.add_error("graph.frame_range_invalid", "frame_range.end must be greater than or equal to frame_range.start", "frame_range");
        return result;
    }

    RenderExecutionPlan execution;
    execution.render_plan = plan;

    execution.steps.push_back(make_step("resolve-scene", RenderStepKind::ResolveScene, "resolve scene inputs"));
    execution.steps.push_back(make_step("resolve-composition", RenderStepKind::ResolveComposition, "resolve target composition"));
    execution.steps.push_back(make_step("prepare-frame-cache-keys", RenderStepKind::PrepareFrameCacheKeys, "prepare per-frame cache keys"));

    for (std::int64_t frame = plan.frame_range.start; frame <= plan.frame_range.end; ++frame) {
        const FrameCacheKey key = build_frame_cache_key(plan, frame);

        FrameRenderTask task;
        task.frame_number = frame;
        task.cache_key = key;
        task.cacheable = true;
        task.invalidates_when_changed = {
            "composition.id",
            "composition_target",
            "frame_number",
            "composition.width",
            "composition.height",
            "composition.frame_rate",
            "composition.layer_count",
            "composition.background",
            "seed_policy_mode",
            "compatibility_mode",
            "output.profile.name",
            "output.profile.class",
            "output.profile.container",
            "output.profile.video.codec",
            "output.profile.video.pixel_format",
            "output.profile.video.rate_control_mode",
            "output.profile.video.crf",
            "output.profile.color.transfer",
            "output.profile.color.range"
        };
        execution.frame_tasks.push_back(std::move(task));

        RenderGraphStep step;
        step.id = "frame-" + std::to_string(frame) + "-rasterize-2d";
        step.kind = RenderStepKind::Rasterize2DFrame;
        step.label = "rasterize 2D frame";
        step.frame_number = frame;
        step.dependencies = {key.value};
        execution.steps.push_back(std::move(step));
    }

    execution.steps.push_back(make_step("encode-output", RenderStepKind::EncodeOutput, "encode final output"));

    result.value = std::move(execution);
    return result;
}

FrameCacheKey build_frame_cache_key(const RenderPlan& plan, std::int64_t frame_number) {
    return make_key(plan, frame_number);
}

bool frame_cache_entry_matches(const FrameCacheEntry& entry, const FrameCacheKey& expected_key) {
    return entry.key.value == expected_key.value;
}

std::string render_step_kind_string(RenderStepKind kind) {
    switch (kind) {
    case RenderStepKind::ResolveScene:
        return "resolve-scene";
    case RenderStepKind::ResolveComposition:
        return "resolve-composition";
    case RenderStepKind::PrepareFrameCacheKeys:
        return "prepare-frame-cache-keys";
    case RenderStepKind::Rasterize2DFrame:
        return "rasterize-2d-frame";
    case RenderStepKind::EncodeOutput:
        return "encode-output";
    }
    return "unknown";
}

} // namespace tachyon
