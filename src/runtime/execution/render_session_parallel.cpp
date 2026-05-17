#include "tachyon/runtime/execution/session/render_session.h"
#include "tachyon/runtime/execution/frames/frame_executor.h"
#include "tachyon/runtime/execution/planning/render_plan.h"
#include "tachyon/runtime/cache/frame_cache.h"
#include "tachyon/runtime/resource/render_context.h"
#include "tachyon/runtime/resource/surface_pool.h"
#include "tachyon/runtime/cache/disk_cache.h"
#include "tachyon/renderer2d/resource/precomp_cache.h"
#include "tachyon/renderer2d/resource/texture_resolver.h"
#include "tachyon/renderer2d/core/framebuffer.h"
#include "tachyon/output/frame_output_sink.h"
#include "tachyon/core/media/media_interfaces.h"
#include "tachyon/core/media/media_provider.h"
#include "tachyon/runtime/execution/session/render_internal.h"
#include "tachyon/runtime/execution/parallel/taskflow_runtime.h"
#include "tachyon/runtime/core/serialization/tbf_codec.h"
#include "tachyon/core/profiling.h"
#include "tachyon/core/diag/log.h"
#include "tachyon/diagnostics/trace.h"

#include <future>
#include <atomic>
#include <algorithm>
#include <thread>
#include <chrono>
#include <cstring>
#include <cstdlib>
#include <fstream>
#include <mutex>

namespace tachyon {

bool render_trace_enabled() {
    static const bool enabled = []() {
        const char* value = std::getenv("TACHYON_RENDER_TRACE");
        return value != nullptr && value[0] != '\0' && value[0] != '0';
    }();
    return enabled;
}

void render_trace(const std::string& message) {
    if (render_trace_enabled()) {
        static std::ofstream s_trace_file = []() {
            const char* temp_dir = std::getenv("TEMP");
            if (!temp_dir) temp_dir = std::getenv("TMP");
            std::filesystem::path path = temp_dir
                ? std::filesystem::path(temp_dir) / "tachyon_render_run.log"
                : std::filesystem::current_path() / "tachyon_render_run.log";
            return std::ofstream(path, std::ios::out | std::ios::trunc);
        }();

        if (s_trace_file.is_open()) {
            s_trace_file << "[RenderTrace] " << message << "\n";
        }
        diag::trace("[RenderTrace] {}", message);
    }
}

// Forward declarations for serialization helpers removed (using TBFCodec)
using runtime::TBFCodec;

// Frame queue for streaming output - holds frames until they can be written in order
struct FrameQueue {
    std::vector<std::shared_ptr<const renderer2d::Framebuffer>> frames;
    std::vector<bool> ready;
    std::vector<bool> cache_hits;
    std::size_t next_to_write{0};
    std::mutex mutex;
    std::condition_variable cv;
    std::atomic<std::size_t> completed_count{0};
    std::size_t total_frames{0};

    explicit FrameQueue(std::size_t size) : frames(size), ready(size, false), cache_hits(size, false), total_frames(size) {}

    void submit(std::size_t index, std::shared_ptr<const renderer2d::Framebuffer> fb, bool cache_hit) {
        std::lock_guard<std::mutex> lock(mutex);
        frames[index] = std::move(fb);
        cache_hits[index] = cache_hit;
        ready[index] = true;
        cv.notify_one();
    }

    // Returns number of frames written
    template<typename WriteFunc>
    std::size_t write_ready_frames(WriteFunc write_fn, RenderSessionResult& result) {
        std::size_t written = 0;
        std::lock_guard<std::mutex> lock(mutex);
        while (next_to_write < total_frames && ready[next_to_write]) {
            if (frames[next_to_write]) {
                write_fn(next_to_write, frames[next_to_write]);
                frames[next_to_write].reset(); // Release memory immediately
                if (cache_hits[next_to_write]) {
                    ++result.cache_hits;
                } else {
                    ++result.cache_misses;
                }
                ++result.frames_written;
            }
            ++next_to_write;
            ++written;
        }
        if (written > 0) {
            cv.notify_all();
        }
        return written;
    }
};

void render_frames_parallel_internal(
    const CompiledScene& compiled_scene,
    const RenderExecutionPlan& execution_plan,
    FrameCache& cache,
    const ::tachyon::runtime::RenderWorkerBudget& budget,
    ::tachyon::RenderContext& context,
    media::IMediaPrefetcher* prefetcher,
    media::IPlaybackScheduler* scheduler,
    std::vector<ExecutedFrame>& rendered_frames,
    RenderProgressCallback progress_callback,
    CancelFlag* cancel_flag,
    runtime::DiskCacheStore* disk_cache,
    output::FrameOutputSink* sink,
    RenderSessionResult* result,
    std::vector<double>* frame_times_out) {

    const std::size_t task_count = execution_plan.frame_tasks.size();

    // If sink is provided, use streaming mode (memory-efficient)
    // Otherwise, use buffered mode (store all frames in rendered_frames)
    const bool streaming_mode = (sink != nullptr);

    if (!streaming_mode) {
        rendered_frames.resize(task_count);
    }
    if (result) {
        result->frame_diagnostics.resize(task_count);
    }

    const std::size_t thread_count = std::max<std::size_t>(1, std::min(budget.frame_concurrency, task_count));
    std::atomic<std::size_t> next_index{0};
    std::atomic<std::size_t> completed_count{0};
    std::unique_ptr<FrameQueue> frame_queue;

    if (streaming_mode) {
        frame_queue = std::make_unique<FrameQueue>(task_count);
    }

    auto execute_frame_at_index = [&](std::size_t index) {
        if (cancel_flag && cancel_flag->load()) {
            return;
        }

        const auto& task = execution_plan.frame_tasks[index];
        RenderPlan frame_plan = execution_plan.render_plan;

        TACHYON_ZONE("ExecuteFrame");
        TACHYON_TRACE_SCOPE("render.frame");
        TACHYON_TRACE_COUNTER("render.frame_index", static_cast<std::int64_t>(task.frame_number));
        TACHYON_TRACE_COUNTER("render.width", static_cast<std::int64_t>(frame_plan.composition.width));
        TACHYON_TRACE_COUNTER("render.height", static_cast<std::int64_t>(frame_plan.composition.height));

        FrameArena arena;
        FrameExecutor executor(arena, cache, nullptr);
        executor.set_parallel_worker_count(budget.pixel_concurrency);

        ::tachyon::RenderContext local_context = context;
        local_context.prefetcher = prefetcher;
        local_context.scheduler = scheduler;
        local_context.pixel_concurrency = budget.pixel_concurrency;
        local_context.cancel_flag = cancel_flag;

        render_trace(
            "frame start index=" + std::to_string(index) +
            " frame=" + std::to_string(task.frame_number) +
            " t=" + std::to_string(task.time_seconds));

        std::shared_ptr<const renderer2d::Framebuffer> framebuffer;
        bool cache_hit = false;
        ExecutedFrame executed_frame;

        // Try to load from disk cache (checkpoint/resume)
        if (disk_cache) {
            runtime::CacheKey key = runtime::CacheKey::build(
                compiled_scene.scene_hash,
                task.layer_filters.empty() ? "" : task.layer_filters.front(),
                task.time_seconds,
                1, // quality_tier
                "beauty"
            );

            auto cached_data = disk_cache->load(key);
            if (cached_data) {
                framebuffer = TBFCodec::decode_framebuffer(*cached_data);
                if (framebuffer) {
                    cache_hit = true;
                }
            }
        }

        // Render if not loaded from cache
        if (!framebuffer) {
            DataSnapshot snapshot;
            const auto frame_start = std::chrono::high_resolution_clock::now();
            
            {
                TACHYON_TRACE_SCOPE("render.frame.draw");
                executed_frame = executor.execute(compiled_scene, frame_plan, task, snapshot, local_context);
            }
            
            const auto frame_end = std::chrono::high_resolution_clock::now();
            const double render_ms = std::chrono::duration<double, std::milli>(frame_end - frame_start).count();
            executed_frame.diagnostics.add_timing(FrameDiagnostics::kCategoryRender, "frame_execute", render_ms);

            cache_hit = cache_hit || executed_frame.cache_hit;
            render_trace(
                "frame end index=" + std::to_string(index) +
                " frame=" + std::to_string(task.frame_number) +
                " cached=" + std::string(cache_hit ? "1" : "0"));

            if (frame_times_out) {
                (*frame_times_out)[index] = render_ms;
            }

            if (executed_frame.frame) {
                framebuffer = std::move(executed_frame.frame);

                // Save to disk cache for checkpoint/resume
                if (disk_cache) {
                    runtime::CacheKey key = runtime::CacheKey::build(
                        compiled_scene.scene_hash,
                        task.layer_filters.empty() ? "" : task.layer_filters.front(),
                        task.time_seconds,
                        1, // quality_tier
                        "beauty"
                    );

                    auto frame_data = TBFCodec::encode_framebuffer(*framebuffer);
                    disk_cache->store(key, frame_data);
                }
            }
        }

        if (streaming_mode && frame_queue && result) {
            // Streaming mode: submit to queue. The dedicated dispatcher thread will handle the writing.
            frame_queue->submit(index, framebuffer, cache_hit);
            result->frame_diagnostics[index] = executed_frame.diagnostics;
        } else {
            // Buffered mode: store in vector (original behavior)
            executed_frame.frame_number = static_cast<std::int64_t>(task.frame_number);
            executed_frame.frame = framebuffer;
            executed_frame.cache_hit = cache_hit;
            executed_frame.scene_hash = compiled_scene.scene_hash;
            if (result) {
                result->frame_diagnostics[index] = executed_frame.diagnostics;
            }
            rendered_frames[index] = std::move(executed_frame);
        }
        
        // Update progress
        std::size_t completed = 0;
        if (frame_queue) {
            completed = frame_queue->completed_count.fetch_add(1) + 1;
        } else {
            completed = completed_count.fetch_add(1) + 1;
        }
        if (progress_callback) {
            progress_callback(completed, task_count);
        }
        TACHYON_FRAME_MARK;
    };

    std::unique_ptr<std::thread> dispatcher_thread;
    if (streaming_mode && frame_queue) {
        dispatcher_thread = std::make_unique<std::thread>([&]() {
            while (true) {
                {
                    std::unique_lock<std::mutex> lock(frame_queue->mutex);
                    frame_queue->cv.wait(lock, [&]() { 
                        return (frame_queue->next_to_write < task_count && frame_queue->ready[frame_queue->next_to_write]) || 
                               frame_queue->completed_count.load() >= task_count || 
                               (cancel_flag && cancel_flag->load()); 
                    });

                    if ((frame_queue->completed_count.load() >= task_count && frame_queue->next_to_write >= task_count) || 
                        (cancel_flag && cancel_flag->load())) {
                        break;
                    }
                } // Unlock mutex here before calling write_ready_frames

                frame_queue->write_ready_frames([&](std::size_t frame_idx, const std::shared_ptr<const renderer2d::Framebuffer>& fb) {
                    if (fb) {
                        output::OutputFramePacket packet;
                        packet.frame_number = static_cast<std::int64_t>(execution_plan.frame_tasks[frame_idx].frame_number);
                        packet.frame = fb;
                        packet.metadata.time_seconds = execution_plan.frame_tasks[frame_idx].time_seconds;
                        packet.metadata.scene_hash = std::to_string(compiled_scene.scene_hash);
                        packet.metadata.color_space = context.cms.output_profile.to_string();
                        
                        const auto write_start = std::chrono::high_resolution_clock::now();
                        bool write_ok = false;
                        {
                            TACHYON_TRACE_SCOPE("render.frame.encode");
                            write_ok = sink->write_frame(packet);
                        }
                        const auto write_end = std::chrono::high_resolution_clock::now();
                        
                        const double write_ms = std::chrono::duration<double, std::milli>(write_end - write_start).count();
                        result->frame_diagnostics[frame_idx].add_timing(FrameDiagnostics::kCategoryOutputWrite, "sink_write", write_ms);

                        if (!write_ok) {
                            if (result) result->output_error = sink->last_error();
                            if (cancel_flag) cancel_flag->store(true);
                        }
                    }
                }, *result);
            }
        });
    }

#if defined(TACHYON_ENABLE_TASKFLOW)
    runtime::TaskflowRuntime tf_runtime(thread_count);
    tf_runtime.parallel_for_frames(task_count, execute_frame_at_index);
#else
    std::vector<std::future<void>> workers;
    workers.reserve(thread_count);

    for (std::size_t worker_index = 0; worker_index < thread_count; ++worker_index) {
        workers.push_back(std::future<void>(std::async(std::launch::async, [&]() {
            while (true) {
                std::size_t index = next_index.fetch_add(1);
                if (index >= task_count || (cancel_flag && cancel_flag->load())) {
                    break;
                }
                execute_frame_at_index(index);
            }
        })));
    }

    for (auto& worker : workers) {
        worker.wait();
    }
#endif

    if (dispatcher_thread && dispatcher_thread->joinable()) {
        dispatcher_thread->join();
    }
}

} // namespace tachyon
