#include "tachyon/runtime/render_session.h"

#include "tachyon/output/frame_output_sink.h"
#include "tachyon/runtime/render_context.h"

#include <algorithm>
#include <atomic>
#include <future>
#include <memory>
#include <vector>

namespace tachyon {
namespace {

void render_frames_sequential(
    const SceneSpec& scene,
    const RenderExecutionPlan& execution_plan,
    FrameCache& cache,
    RenderContext& context,
    std::vector<ExecutedFrame>& rendered_frames) {
    rendered_frames.reserve(execution_plan.frame_tasks.size());
    for (const auto& task : execution_plan.frame_tasks) {
        rendered_frames.push_back(execute_frame_task(scene, execution_plan.render_plan, task, cache, context));
    }
}

void render_frames_parallel(
    const SceneSpec& scene,
    const RenderExecutionPlan& execution_plan,
    FrameCache& cache,
    std::size_t worker_count,
    RenderContext& context,
    std::vector<ExecutedFrame>& rendered_frames) {

    const std::size_t task_count = execution_plan.frame_tasks.size();
    rendered_frames.resize(task_count);
    const std::size_t thread_count = std::max<std::size_t>(1, std::min(worker_count, task_count));
    std::atomic<std::size_t> next_index{0};
    std::vector<std::future<void>> workers;
    workers.reserve(thread_count);

    for (std::size_t worker_index = 0; worker_index < thread_count; ++worker_index) {
        workers.push_back(std::async(std::launch::async, [&]() {
            for (;;) {
                const std::size_t index = next_index.fetch_add(1);
                if (index >= task_count) {
                    return;
                }

                rendered_frames[index] = execute_frame_task(scene, execution_plan.render_plan, execution_plan.frame_tasks[index], cache, context);
            }
        }));
    }

    for (auto& worker : workers) {
        worker.get();
    }
}

} // namespace

RenderSessionResult RenderSession::render(
    const SceneSpec& scene,
    const RenderExecutionPlan& execution_plan,
    const std::filesystem::path& output_path) {
    return render(scene, execution_plan, output_path, 1);
}

RenderSessionResult RenderSession::render(
    const SceneSpec& scene,
    const RenderExecutionPlan& execution_plan,
    const std::filesystem::path& output_path,
    std::size_t worker_count) {
    RenderSessionResult result;
    RenderContext context(m_precomp_cache);
    context.policy = make_quality_policy(execution_plan.render_plan.quality_tier);
    context.renderer2d.policy = context.policy;
    if (context.renderer2d.precomp_cache) {
        context.renderer2d.precomp_cache->max_bytes = context.policy.precomp_cache_budget;
    }

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

    std::vector<ExecutedFrame> rendered_frames;
    if (worker_count <= 1 || execution_plan.frame_tasks.size() <= 1) {
        render_frames_sequential(scene, execution_plan, m_cache, context, rendered_frames);
    } else {
        render_frames_parallel(scene, execution_plan, m_cache, worker_count, context, rendered_frames);
    }

    for (ExecutedFrame& frame : rendered_frames) {
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

    result.diagnostics = context.media.consume_diagnostics();
    return result;
}

} // namespace tachyon
