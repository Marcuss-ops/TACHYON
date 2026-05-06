#include "frame_executor_internal.h"

#include <algorithm>

namespace tachyon {

FrameTimingState resolve_frame_timing(
    const CompiledScene& compiled_scene,
    const RenderPlan& plan,
    const FrameRenderTask& task) {
    FrameTimingState timing;
    timing.fps = compiled_scene.compositions.empty() ? 60.0 : std::max(1u, compiled_scene.compositions.front().fps);
    timing.frame_time_seconds = static_cast<double>(task.frame_number) / timing.fps;

    if (plan.time_remap_curve.has_value()) {
        const float dest_time = static_cast<float>(timing.frame_time_seconds);
        timing.frame_time_seconds = static_cast<double>(timeline::evaluate_source_time(*plan.time_remap_curve, dest_time));
        const float frame_duration = static_cast<float>(1.0 / timing.fps);
        timing.blend_result = timeline::evaluate_frame_blend(*plan.time_remap_curve, dest_time, frame_duration);
    }

    return timing;
}

} // namespace tachyon
