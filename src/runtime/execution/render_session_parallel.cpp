#include "tachyon/runtime/execution/session/render_session.h"
#include "tachyon/runtime/execution/frames/frame_executor.h"
#include "tachyon/runtime/execution/planning/render_plan.h"
#include "tachyon/runtime/cache/frame_cache.h"
#include "tachyon/runtime/telemetry/thread_local_telemetry.h"
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
#include "tachyon/core/render_telemetry.h"
#include <condition_variable>

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

    // Always resize rendered_frames to preserve shell frame metadata for database logging & statistics
    rendered_frames.resize(task_count);
    if (result) {
        result->frame_diagnostics.resize(task_count);
    }

    // Thread-safe accumulators for processed pixels and tiles
    auto pixel_ops_counter = std::make_shared<std::atomic<std::size_t>>(0);
    auto rasterized_pixels_counter = std::make_shared<std::atomic<std::size_t>>(0);
    auto blend_pixels_counter = std::make_shared<std::atomic<std::size_t>>(0);
    auto encoded_pixels_counter = std::make_shared<std::atomic<std::size_t>>(0);
    auto tiles_counter = std::make_shared<std::atomic<int>>(0);

    const std::size_t thread_count = std::max<std::size_t>(1, std::min(budget.frame_concurrency, task_count));
    std::atomic<std::size_t> next_index{0};
    std::atomic<std::size_t> completed_count{0};
    std::unique_ptr<FrameQueue> frame_queue;

    if (streaming_mode) {
        frame_queue = std::make_unique<FrameQueue>(task_count);
    }

    std::mutex bake_mutex;
    std::condition_variable bake_cv;
    std::shared_ptr<const renderer2d::Framebuffer> baked_frame;
    bool baked_frame_ready = false;
    double bake_build_ms = 0.0;
    std::atomic<std::size_t> bake_hits{0};
    std::atomic<std::size_t> bake_misses{0};

    auto execute_frame_at_index = [&](std::size_t index) {
        if (cancel_flag && cancel_flag->load()) {
            return;
        }

        const auto& task = execution_plan.frame_tasks[index];
        RenderPlan frame_plan = execution_plan.render_plan;

        std::shared_ptr<const renderer2d::Framebuffer> framebuffer;
        bool cache_hit = false;
        ExecutedFrame executed_frame;

        if (context.static_bake_proof) {
            if (index == 0) {
                const auto bake_start = std::chrono::high_resolution_clock::now();
                
                FrameArena arena;
                FrameExecutor executor(arena, cache, nullptr);
                executor.set_parallel_worker_count(budget.pixel_concurrency);

                ::tachyon::RenderContext local_context = context;
#ifdef TACHYON_ENABLE_MEDIA
                local_context.prefetcher = prefetcher;
                local_context.scheduler = scheduler;
#endif
                local_context.pixel_concurrency = budget.pixel_concurrency;
                local_context.cancel_flag = cancel_flag;
                local_context.total_pixel_ops_counter = pixel_ops_counter;
                local_context.rasterized_pixels_counter = rasterized_pixels_counter;
                local_context.blend_pixels_counter = blend_pixels_counter;
                local_context.encoded_pixels_counter = encoded_pixels_counter;
                local_context.total_tiles_counter = tiles_counter;

                {
                    TACHYON_ZONE("ExecuteFrameBake");
                    DataSnapshot snapshot;
                    executed_frame = executor.execute(compiled_scene, frame_plan, task, snapshot, local_context);
                }

                if (!executed_frame.cache_hit && !executed_frame.diagnostics.has_category(FrameDiagnostics::kCategoryRender, "frame_blend")) {
                    const std::uint64_t pixels = static_cast<std::uint64_t>(frame_plan.composition.width) * frame_plan.composition.height;
                    runtime::tl_telemetry.pixel_ops += pixels;
                    runtime::tl_telemetry.rasterized_pixels += pixels;
                    
                    int tile_size = frame_plan.quality_policy.tile_size;
                    if (tile_size > 0) {
                        int tiles_x = (frame_plan.composition.width + tile_size - 1) / tile_size;
                        int tiles_y = (frame_plan.composition.height + tile_size - 1) / tile_size;
                        runtime::tl_telemetry.tiles_processed += static_cast<std::uint64_t>(tiles_x) * tiles_y;
                    } else {
                        runtime::tl_telemetry.tiles_processed += 1;
                    }
                }

                const auto bake_end = std::chrono::high_resolution_clock::now();
                bake_build_ms = std::chrono::duration<double, std::milli>(bake_end - bake_start).count();
                executed_frame.diagnostics.add_timing(FrameDiagnostics::kCategoryRender, "frame_execute", bake_build_ms);

                if (frame_times_out) {
                    (*frame_times_out)[index] = bake_build_ms;
                }

                if (executed_frame.frame) {
                    framebuffer = executed_frame.frame;
                }

                {
                    std::lock_guard<std::mutex> lock(bake_mutex);
                    baked_frame = framebuffer;
                    baked_frame_ready = true;
                    bake_misses++;
                }
                bake_cv.notify_all();
            } else {
                std::shared_ptr<const renderer2d::Framebuffer> fb;
                {
                    std::unique_lock<std::mutex> lock(bake_mutex);
                    bake_cv.wait(lock, [&] { return baked_frame_ready; });
                    fb = baked_frame;
                    bake_hits++;
                }
                framebuffer = fb;
                cache_hit = true;
            }
        } else {
            TACHYON_ZONE("ExecuteFrame");
            TACHYON_TRACE_SCOPE("render.frame");
            TACHYON_TRACE_COUNTER("render.frame_index", static_cast<std::int64_t>(task.frame_number));
            TACHYON_TRACE_COUNTER("render.width", static_cast<std::int64_t>(frame_plan.composition.width));
            TACHYON_TRACE_COUNTER("render.height", static_cast<std::int64_t>(frame_plan.composition.height));

            FrameArena arena;
            FrameExecutor executor(arena, cache, nullptr);
            executor.set_parallel_worker_count(budget.pixel_concurrency);

            ::tachyon::RenderContext local_context = context;
#ifdef TACHYON_ENABLE_MEDIA
            local_context.prefetcher = prefetcher;
            local_context.scheduler = scheduler;
#endif
            local_context.pixel_concurrency = budget.pixel_concurrency;
            local_context.cancel_flag = cancel_flag;
            local_context.total_pixel_ops_counter = pixel_ops_counter;
            local_context.rasterized_pixels_counter = rasterized_pixels_counter;
            local_context.blend_pixels_counter = blend_pixels_counter;
            local_context.encoded_pixels_counter = encoded_pixels_counter;
            local_context.total_tiles_counter = tiles_counter;

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

                // Track rasterized pixels and tiles processed for cache miss execution path via thread-local telemetry.
                // Note: If this was a blended frame, telemetry is already handled inside try_render_blend.
                if (!executed_frame.cache_hit && !executed_frame.diagnostics.has_category(FrameDiagnostics::kCategoryRender, "frame_blend")) {
                    const std::uint64_t pixels = static_cast<std::uint64_t>(frame_plan.composition.width) * frame_plan.composition.height;
                    runtime::tl_telemetry.pixel_ops += pixels;
                    runtime::tl_telemetry.rasterized_pixels += pixels;
                    
                    int tile_size = frame_plan.quality_policy.tile_size;
                    if (tile_size > 0) {
                        int tiles_x = (frame_plan.composition.width + tile_size - 1) / tile_size;
                        int tiles_y = (frame_plan.composition.height + tile_size - 1) / tile_size;
                        runtime::tl_telemetry.tiles_processed += static_cast<std::uint64_t>(tiles_x) * tiles_y;
                    } else {
                        runtime::tl_telemetry.tiles_processed += 1;
                    }
                }
                
                const auto frame_end = std::chrono::high_resolution_clock::now();
                const double render_ms = std::chrono::duration<double, std::milli>(frame_end - frame_start).count();
                executed_frame.diagnostics.add_timing(FrameDiagnostics::kCategoryRender, "frame_execute", render_ms);

                cache_hit = cache_hit || executed_frame.cache_hit;

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
        }

        if (framebuffer) {
            // Track encoded/finalized pixels
            runtime::tl_telemetry.encoded_pixels += static_cast<std::uint64_t>(framebuffer->width()) * framebuffer->height();
        }

        if (streaming_mode && frame_queue && result) {
            // Streaming mode: submit to queue. The dedicated dispatcher thread will handle the writing.
            frame_queue->submit(index, framebuffer, cache_hit);
            result->frame_diagnostics[index] = executed_frame.diagnostics;

            // Populate lightweight metadata shell in rendered_frames to preserve Zero Waste RAM footprints
            ExecutedFrame shell_frame;
            shell_frame.frame_number = static_cast<std::int64_t>(task.frame_number);
            shell_frame.frame = nullptr; // keep frame buffer nullptr to prevent RAM growth
            shell_frame.cache_hit = cache_hit;
            shell_frame.scene_hash = compiled_scene.scene_hash;
            shell_frame.diagnostics = executed_frame.diagnostics;
            rendered_frames[index] = std::move(shell_frame);
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
        runtime::flush_thread_local_telemetry(pixel_ops_counter, tiles_counter, rasterized_pixels_counter, blend_pixels_counter, encoded_pixels_counter);
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

    // Set aggregate telemetry metrics in the result structure
    if (result) {
        result->total_pixel_ops = pixel_ops_counter->load();
        result->rasterized_pixels = rasterized_pixels_counter->load();
        result->blend_pixel_ops = blend_pixels_counter->load();
        result->encoded_pixels = encoded_pixels_counter->load();
        result->total_tiles = tiles_counter->load();
    }

    if (context.static_bake_proof) {
        std::size_t baked_bytes = 0;
        if (baked_frame) {
            baked_bytes = (baked_frame->pixels().size() + baked_frame->depth_buffer().size()) * sizeof(float);
        }
        RenderTelemetry::get().set_static_bake_info(true, bake_hits.load(), bake_misses.load(), baked_bytes, bake_build_ms);
    }
}

} // namespace tachyon
