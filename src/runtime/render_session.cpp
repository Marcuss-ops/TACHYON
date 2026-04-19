#include "tachyon/runtime/render_session.h"

#include "tachyon/output/frame_output_sink.h"

#include <memory>

namespace tachyon {

RenderSessionResult RenderSession::render(
    const SceneSpec& scene,
    const RenderExecutionPlan& execution_plan,
    const std::filesystem::path& output_path) {

    RenderSessionResult result;

    RenderPlan effective_plan = execution_plan.render_plan;
    if (!output_path.empty()) {
        effective_plan.output.destination.path = output_path.string();
    }

    std::unique_ptr<output::FrameOutputSink> sink = output::create_frame_output_sink(effective_plan);
    if (sink) {
        result.output_configured = true;
        if (!sink->begin(effective_plan)) {
            result.output_error = sink->last_error();
            return result;
        }
    }

    for (const auto& task : execution_plan.frame_tasks) {
        ExecutedFrame frame = execute_frame_task(scene, effective_plan, task, m_cache);
        if (frame.cache_hit) {
            ++result.cache_hits;
        } else {
            ++result.cache_misses;
        }

        if (sink) {
            const output::OutputFramePacket packet{frame.frame_number, &frame.frame};
            if (!sink->write_frame(packet)) {
                result.output_error = sink->last_error();
            } else {
                ++result.frames_written;
            }
        }

        result.frames.push_back(std::move(frame));
        if (!result.output_error.empty()) {
            break;
        }
    }

    if (sink && result.output_error.empty() && !sink->finish()) {
        result.output_error = sink->last_error();
    }

    return result;
}

} // namespace tachyon
