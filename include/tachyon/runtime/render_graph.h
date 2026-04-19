#pragma once

#include "tachyon/runtime/diagnostics.h"
#include "tachyon/runtime/render_plan.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace tachyon {

enum class RenderStepKind {
    ValidateContracts,
    ResolveScene,
    ResolveComposition,
    PrepareFrameCacheKeys,
    Rasterize2DFrame,
    EncodeOutput
};

struct RenderGraphStep {
    std::string id;
    RenderStepKind kind{RenderStepKind::ResolveScene};
    std::string label;
    std::optional<std::int64_t> frame_number;
    std::vector<std::string> dependencies;
};

struct FrameCacheKey {
    std::string value;
};

struct FrameCacheEntry {
    FrameCacheKey key;
    std::string note;
};

struct FrameRenderTask {
    std::int64_t frame_number{0};
    FrameCacheKey cache_key;
    bool cacheable{true};
    std::vector<std::string> invalidates_when_changed;
};

struct RenderExecutionPlan {
    RenderPlan render_plan;
    std::size_t resolved_asset_count{0};
    std::vector<RenderGraphStep> steps;
    std::vector<FrameRenderTask> frame_tasks;
};

ResolutionResult<RenderExecutionPlan> build_render_execution_plan(const RenderPlan& plan, std::size_t resolved_asset_count = 0);
FrameCacheKey build_frame_cache_key(const RenderPlan& plan, std::int64_t frame_number);
bool frame_cache_entry_matches(const FrameCacheEntry& entry, const FrameCacheKey& expected_key);
std::string render_step_kind_string(RenderStepKind kind);

} // namespace tachyon
