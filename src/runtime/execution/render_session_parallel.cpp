#include "tachyon/runtime/execution/session/render_session.h"
#include "tachyon/runtime/execution/frames/frame_executor.h"
#include "tachyon/runtime/execution/planning/render_plan.h"
#include "tachyon/runtime/cache/frame_cache.h"
#include "tachyon/runtime/resource/render_context.h"
#include "tachyon/runtime/resource/runtime_surface_pool.h"
#include "tachyon/runtime/cache/disk_cache.h"
#include "tachyon/renderer2d/resource/precomp_cache.h"
#include "tachyon/renderer2d/resource/texture_resolver.h"
#include "tachyon/renderer2d/core/framebuffer.h"
#include "tachyon/output/frame_output_sink.h"
#include "tachyon/media/streaming/media_prefetcher.h"
#include "tachyon/runtime/execution/session/render_internal.h"

#ifdef TACHYON_TRACY_ENABLED
#include <tracy/Tracy.hpp>
#endif

#include <iostream>
#include <future>
#include <atomic>
#include <algorithm>
#include <thread>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <cstring>
#include <cstdlib>

namespace tachyon {

// Forward declarations for serialization helpers
std::vector<uint8_t> serialize_framebuffer(const renderer2d::Framebuffer& fb);
std::shared_ptr<renderer2d::Framebuffer> deserialize_framebuffer(const std::vector<uint8_t>& data);

// Frame queue for streaming output - holds frames until they can be written in order
struct FrameQueue {
    std::vector<std::shared_ptr<renderer2d::Framebuffer>> frames;
    std::vector<bool> ready;
    std::vector<bool> cache_hits;
    std::size_t next_to_write{0};
    std::mutex mutex;
    std::condition_variable cv;
    std::atomic<std::size_t> completed_count{0};
    std::size_t total_frames{0};

    explicit FrameQueue(std::size_t size) : frames(size), ready(size, false), cache_hits(size, false), total_frames(size) {}

    void submit(std::size_t index, std::shared_ptr<renderer2d::Framebuffer> fb, bool cache_hit) {
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
    std::size_t worker_count,
    ::tachyon::RenderContext& context,
    media::MediaPrefetcher& prefetcher,
    media::PlaybackScheduler* scheduler,
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

    const std::size_t thread_count = std::max<std::size_t>(1, std::min(worker_count, task_count));
    std::atomic<std::size_t> next_index{0};
    std::atomic<std::size_t> completed_count{0};
    std::unique_ptr<FrameQueue> frame_queue;

    if (streaming_mode) {
        frame_queue = std::make_unique<FrameQueue>(task_count);
    }

    std::vector<std::future<void>> workers;
    workers.reserve(thread_count);

    for (std::size_t worker_index = 0; worker_index < thread_count; ++worker_index) {
        workers.push_back(std::future<void>(std::async(std::launch::async, [&, worker_index]() {
            FrameArena arena;
            FrameExecutor executor(arena, cache, nullptr);
            executor.set_parallel_worker_count(thread_count);

             // Create a thread-local context
              ::tachyon::RenderContext local_context(context.renderer2d.precomp_cache);
              local_context.media = context.media;
              local_context.prefetcher = &prefetcher;
              local_context.scheduler = scheduler;
              local_context.ray_tracer = context.ray_tracer;
              local_context.policy = context.policy;
              local_context.renderer2d.policy = context.policy;
              local_context.surface_pool = context.surface_pool;
              local_context.renderer2d.font_registry = context.renderer2d.font_registry;
              local_context.renderer2d.media_manager = context.renderer2d.media_manager;
              local_context.renderer2d.cms = context.renderer2d.cms;
              local_context.renderer2d.diagnostics = context.renderer2d.diagnostics;
              local_context.cancel_flag = cancel_flag;
              local_context.renderer2d.cancel_flag = cancel_flag;

            for (;;) {
                // Check for cancellation
                if (cancel_flag && cancel_flag->load()) {
                    return;
                }

#ifdef TACHYON_TRACY_ENABLED
                ZoneScopedN("RenderSession::WorkerLoop::Frame");
#endif

                const std::size_t index = next_index.fetch_add(1);
                if (index >= task_count) {
                    return;
                }

                const auto& task = execution_plan.frame_tasks[index];
                RenderPlan frame_plan = execution_plan.render_plan;

                std::shared_ptr<renderer2d::Framebuffer> framebuffer;
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
                        framebuffer = deserialize_framebuffer(*cached_data);
                        if (framebuffer) {
                            cache_hit = true;
                        }
                    }
                }

                // Render if not loaded from cache
                if (!framebuffer) {
                    DataSnapshot snapshot;
                    const auto frame_start = std::chrono::high_resolution_clock::now();
                    
                    executed_frame = executor.execute(compiled_scene, frame_plan, task, snapshot, local_context);
                    cache_hit = cache_hit || executed_frame.cache_hit;

                    const auto frame_end = std::chrono::high_resolution_clock::now();
                    if (frame_times_out) {
                        (*frame_times_out)[index] = std::chrono::duration<double, std::milli>(frame_end - frame_start).count();
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

                            auto frame_data = serialize_framebuffer(*framebuffer);
                            disk_cache->store(key, frame_data);
                        }
                    }
                }

                if (streaming_mode && frame_queue && result) {
                    // Streaming mode: submit to queue for immediate writing
                    frame_queue->submit(index, framebuffer, cache_hit);
                    result->frame_diagnostics[index] = executed_frame.diagnostics;

                    // Try to write ready frames in order
                    frame_queue->write_ready_frames([&](std::size_t frame_idx, const std::shared_ptr<renderer2d::Framebuffer>& fb) {
                        if (fb) {
                            output::OutputFramePacket packet;
                            packet.frame_number = static_cast<std::int64_t>(execution_plan.frame_tasks[frame_idx].frame_number);
                            packet.frame = fb.get();
                            packet.metadata.time_seconds = execution_plan.frame_tasks[frame_idx].time_seconds;
                            packet.metadata.scene_hash = std::to_string(compiled_scene.scene_hash);
                            packet.metadata.color_space = context.renderer2d.cms.output_profile.to_string();
                            if (!sink->write_frame(packet)) {
                                if (result) {
                                    result->output_error = sink->last_error();
                                }
                                if (cancel_flag) {
                                    cancel_flag->store(true);
                                }
                            }
                        }
                    }, *result);
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
            }
        })));
    }

    for (auto& w : workers) {
        w.wait();
    }

    // In streaming mode, wait for any remaining frames to be written
    if (streaming_mode && frame_queue && sink && result) {
        // Wait for all frames to be written
        std::unique_lock<std::mutex> lock(frame_queue->mutex);
        frame_queue->cv.wait(lock, [&]() { return frame_queue->next_to_write >= task_count; });
    }
}

} // namespace tachyon
