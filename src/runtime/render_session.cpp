#include "tachyon/runtime/render_session.h"

#include <filesystem>
#include <iomanip>
#include <sstream>

namespace tachyon {
namespace {

std::filesystem::path resolve_frame_output_path(const std::filesystem::path& output_path, std::int64_t frame_number) {
    if (output_path.empty()) {
        return {};
    }

    std::filesystem::path directory = output_path;
    std::string stem = "frame";

    if (output_path.has_extension()) {
        directory = output_path.parent_path();
        stem = output_path.stem().string();
    }

    if (directory.empty()) {
        directory = ".";
    }

    std::ostringstream filename;
    filename << stem << '_' << std::setw(6) << std::setfill('0') << frame_number << ".png";
    return directory / filename.str();
}

} // namespace

RenderSessionResult RenderSession::render(
    const SceneSpec& scene,
    const RenderExecutionPlan& execution_plan,
    const std::filesystem::path& output_path) {

    RenderSessionResult result;
    for (const auto& task : execution_plan.frame_tasks) {
        ExecutedFrame frame = execute_frame_task(scene, execution_plan.render_plan, task, m_cache);
        if (frame.cache_hit) {
            ++result.cache_hits;
        } else {
            ++result.cache_misses;
        }

        const std::filesystem::path frame_output_path = resolve_frame_output_path(output_path, frame.frame_number);
        if (!frame_output_path.empty()) {
            frame.frame.save_png(frame_output_path);
        }

        result.frames.push_back(std::move(frame));
    }

    return result;
}

} // namespace tachyon
