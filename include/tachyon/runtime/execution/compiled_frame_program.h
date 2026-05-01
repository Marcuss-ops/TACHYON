#pragma once

#include "tachyon/core/spec/schema/objects/scene_spec.h"
#include "tachyon/runtime/core/data/compiled_scene.h"
#include "tachyon/runtime/execution/planning/render_plan.h"
#include <memory>
#include <string>
#include <vector>

namespace tachyon {

struct RenderSessionResult; // defined in render_session.h


/**
 * Precompiled frame rendering program that caches compiled resources
 * to avoid per-frame overhead, enabling 100x performance improvements.
 */
struct CompiledFrameProgram {
    SceneSpec scene;
    CompiledScene compiled_scene;
    RenderExecutionPlan exec_plan;
    std::vector<uint8_t> frame_cache;

    /**
     * Execute the precompiled program to render a frame at the given time.
     * Avoids recompilation and resource setup per frame.
     */
    [[nodiscard]] RenderSessionResult render_frame(double time_sec, const std::string& output_path) const;
};

/**
 * Build a CompiledFrameProgram from a scene spec and execution plan.
 * Precompiles all necessary resources for fast frame rendering.
 */
[[nodiscard]] ResolutionResult<CompiledFrameProgram> build_compiled_frame_program(
    const SceneSpec& scene,
    const RenderExecutionPlan& plan
);

} // namespace tachyon
