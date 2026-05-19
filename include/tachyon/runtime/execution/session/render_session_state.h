#pragma once

#include "tachyon/runtime/execution/planning/render_plan.h"
#include "tachyon/runtime/resource/render_context.h"
#include "tachyon/output/frame_output_sink.h"
#include "tachyon/output/output_planner.h"
#include "tachyon/runtime/execution/frames/frame_executor.h"
#include <chrono>
#include <memory>
#include <string>
#include <vector>
#include <filesystem>

namespace tachyon {

struct RenderSessionState {
    RenderExecutionPlan effective_plan;
    std::string resolved_output_path;
    RenderContext context;

    std::unique_ptr<output::FrameOutputSink> sink;
    output::AudioExportPlan audio_plan;

#ifdef TACHYON_ENABLE_MEDIA
    media::IMediaPrefetcher* prefetcher{nullptr};
#endif

    std::vector<double> frame_times;
    std::vector<ExecutedFrame> rendered_frames;

    std::chrono::steady_clock::time_point session_start;
    std::chrono::high_resolution_clock::time_point frame_exec_start;
    std::chrono::high_resolution_clock::time_point frame_exec_end;
};

class RenderSessionPlanResolver {
public:
    static std::string resolve_output_path(
        const std::filesystem::path& output_path,
        const RenderPlan& plan
    );

    static void apply_output_override(
        RenderSessionState& state,
        const std::filesystem::path& output_path
    );
};

} // namespace tachyon
