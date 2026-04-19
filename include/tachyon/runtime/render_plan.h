#pragma once

#include "tachyon/runtime/diagnostics.h"
#include "tachyon/runtime/render_job.h"
#include "tachyon/spec/scene_spec.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>

namespace tachyon {

struct CompositionSummary {
    std::string id;
    std::string name;
    std::int64_t width{0};
    std::int64_t height{0};
    double duration{0.0};
    FrameRate frame_rate;
    std::optional<std::string> background;
    std::size_t layer_count{0};
};

struct RenderPlan {
    std::string job_id;
    std::string scene_ref;
    std::string composition_target;
    CompositionSummary composition;
    FrameRange frame_range;
    OutputContract output;
    std::string seed_policy_mode;
    std::string compatibility_mode;
};

ResolutionResult<RenderPlan> build_render_plan(const SceneSpec& scene, const RenderJob& job);

} // namespace tachyon
