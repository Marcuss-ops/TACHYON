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
#include "tachyon/audio/audio_export.h"

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

// Helper functions for framebuffer serialization (for checkpoint/resume)
namespace {

std::vector<uint8_t> serialize_framebuffer(const renderer2d::Framebuffer& fb) {
    std::vector<uint8_t> data;
    const uint32_t width = fb.width();
    const uint32_t height = fb.height();
    const auto& pixels = fb.pixels();

    // Header: width (4 bytes) + height (4 bytes)
    data.resize(8 + pixels.size() * sizeof(float));
    std::memcpy(data.data(), &width, 4);
    std::memcpy(data.data() + 4, &height, 4);
    std::memcpy(data.data() + 8, pixels.data(), pixels.size() * sizeof(float));

    return data;
}

std::shared_ptr<renderer2d::Framebuffer> deserialize_framebuffer(const std::vector<uint8_t>& data) {
    if (data.size() < 8) return nullptr;

    uint32_t width = 0, height = 0;
    std::memcpy(&width, data.data(), 4);
    std::memcpy(&height, data.data() + 4, 4);

    const size_t expected_pixel_bytes = width * height * 4 * sizeof(float);
    if (data.size() < 8 + expected_pixel_bytes) return nullptr;

    auto fb = std::make_shared<renderer2d::Framebuffer>(width, height);
    std::memcpy(fb->mutable_pixels().data(), data.data() + 8, expected_pixel_bytes);

    return fb;
}

} // namespace

// Mux audio and video using FFmpeg command line
bool mux_audio_video(const std::string& video_path, const std::string& audio_path, std::string& error) {
    // Check if input files exist
    if (!std::filesystem::exists(video_path)) {
        error = "Video file not found: " + video_path;
        return false;
    }
    if (!std::filesystem::exists(audio_path)) {
        error = "Audio file not found: " + audio_path;
        return false;
    }

    // Create output path (temp file)
    std::filesystem::path video_p(video_path);
    std::filesystem::path output_path = video_p.parent_path() / ("muxed_" + video_p.filename().string());

    // Build FFmpeg command to mux audio and video
    // -y: overwrite output file
    // -i video_path: input video
    // -i audio_path: input audio
    // -c:v copy: copy video codec (no re-encode)
    // -c:a aac: encode audio to AAC
    // -map 0:v:0: use video from first input
    // -map 1:a:0: use audio from second input
    std::string command = "ffmpeg -y -i \"" + video_path + "\" -i \"" + audio_path + "\" -c:v copy -c:a aac -map 0:v:0 -map 1:a:0 \"" + output_path.string() + "\"";

    int result = std::system(command.c_str());
    if (result != 0) {
        error = "FFmpeg mux failed with code: " + std::to_string(result);
        return false;
    }

    // Replace original video with muxed version
    try {
        std::filesystem::remove(video_path);
        std::filesystem::rename(output_path, video_path);
    } catch (const std::exception& e) {
        error = std::string("Failed to replace video file: ") + e.what();
        return false;
    }

    return true;
}

output::OutputFramePacket make_output_packet(const ExecutedFrame& frame, double fps) {
    output::OutputFramePacket packet;
    packet.frame_number = frame.frame_number;
    packet.frame = frame.frame.get();
    packet.metadata.time_seconds = frame.frame_number / fps;
    packet.metadata.scene_hash = std::to_string(frame.scene_hash);
    packet.metadata.color_space = "linear_rec709";
    return packet;
}

std::string output_path_or_plan(const std::filesystem::path& output_path, const RenderPlan& plan) {
    if (!output_path.empty()) {
        return output_path.string();
    }
    return plan.output.destination.path;
}

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
        return written;
    }
};

void render_frames_parallel(
    const CompiledScene& compiled_scene,
    const RenderExecutionPlan& execution_plan,
    FrameCache& cache,
    std::size_t worker_count,
    ::tachyon::RenderContext& context,
    media::MediaPrefetcher& prefetcher,
    media::PlaybackScheduler* scheduler,
    const CompiledScene& original_scene,
    double session_fps,
    std::vector<ExecutedFrame>& rendered_frames,
    RenderProgressCallback progress_callback = nullptr,
    CancelFlag* cancel_flag = nullptr,
    runtime::DiskCacheStore* disk_cache = nullptr,
    output::FrameOutputSink* sink = nullptr,
    RenderSessionResult* result = nullptr) {

    (void)original_scene;
    (void)session_fps;
    (void)scheduler;
    (void)prefetcher;

    const std::size_t task_count = execution_plan.frame_tasks.size();

    // If sink is provided, use streaming mode (memory-efficient)
    // Otherwise, use buffered mode (store all frames in rendered_frames)
    const bool streaming_mode = (sink != nullptr);

    if (!streaming_mode) {
        rendered_frames.resize(task_count);
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

             // Create a thread-local context
             ::tachyon::RenderContext local_context(context.renderer2d.precomp_cache);
             local_context.media = context.media;
             local_context.ray_tracer = context.ray_tracer;
             local_context.policy = context.policy;
             local_context.surface_pool = context.surface_pool;
             local_context.renderer2d.font_registry = context.renderer2d.font_registry;
             local_context.renderer2d.media_manager = context.renderer2d.media_manager;

            for (;;) {
                // Check for cancellation
                if (cancel_flag && cancel_flag->load()) {
                    return;
                }

                const std::size_t index = next_index.fetch_add(1);
                if (index >= task_count) {
                    return;
                }

                const auto& task = execution_plan.frame_tasks[index];
                RenderPlan frame_plan = execution_plan.render_plan;

                std::shared_ptr<renderer2d::Framebuffer> framebuffer;
                bool cache_hit = false;

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
                    auto executed_frame = executor.execute(compiled_scene, frame_plan, task, snapshot, local_context);

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

                    // Try to write ready frames in order
                    frame_queue->write_ready_frames([&](std::size_t frame_idx, const std::shared_ptr<renderer2d::Framebuffer>& fb) {
                        if (fb) {
                            output::OutputFramePacket packet;
                            packet.frame_number = static_cast<std::int64_t>(execution_plan.frame_tasks[frame_idx].frame_number);
                            packet.frame = fb.get();
                            packet.metadata.time_seconds = execution_plan.frame_tasks[frame_idx].time_seconds;
                            packet.metadata.scene_hash = std::to_string(compiled_scene.scene_hash);
                            packet.metadata.color_space = "linear_rec709";
                            sink->write_frame(packet);
                        }
                    }, *result);
                } else {
                    // Buffered mode: store in vector (original behavior)
                    ExecutedFrame executed_frame;
                    executed_frame.frame_number = static_cast<std::int64_t>(task.frame_number);
                    executed_frame.frame = framebuffer;
                    executed_frame.cache_hit = cache_hit;
                    executed_frame.scene_hash = compiled_scene.scene_hash;
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

RenderSessionResult RenderSession::render(
    const SceneSpec& scene,
    const CompiledScene& compiled_scene,
    const RenderExecutionPlan& execution_plan,
    const std::filesystem::path& output_path,
    std::size_t worker_count) {

    (void)scene;
    RenderSessionResult result;

    // Start timing the entire render session
    auto session_start = std::chrono::high_resolution_clock::now();

    RenderExecutionPlan effective_plan = execution_plan;
    const std::string resolved_output_path = output_path_or_plan(output_path, execution_plan.render_plan);
    if (!resolved_output_path.empty()) {
        effective_plan.render_plan.output.destination.path = resolved_output_path;
    }

    ::tachyon::RenderContext context(m_precomp_cache);
    context.renderer2d.font_registry = ::tachyon::renderer2d::get_default_font_registry();
    std::uint32_t w = static_cast<std::uint32_t>(execution_plan.render_plan.composition.width);
    std::uint32_t h = static_cast<std::uint32_t>(execution_plan.render_plan.composition.height);
    m_surface_pool = std::make_unique<runtime::RuntimeSurfacePool>(w, h, 10);
    context.surface_pool = m_surface_pool.get();

    media::MediaPrefetcher prefetcher;

    double fps = execution_plan.render_plan.composition.frame_rate.value();
    if (fps <= 0.0) fps = 24.0;

    // Create output sink and begin streaming mode
    auto encode_start = std::chrono::high_resolution_clock::now();
    std::unique_ptr<output::FrameOutputSink> sink = output::create_frame_output_sink(effective_plan.render_plan);
    if (sink) {
        if (!sink->begin(effective_plan.render_plan)) {
            result.output_error = sink->last_error();
            return result;
        }
        result.output_configured = true;
    }

    const bool streaming_mode = (sink != nullptr);

    // Render frames with streaming to sink (memory-efficient)
    std::vector<ExecutedFrame> rendered_frames; // Kept for metadata compatibility
    render_frames_parallel(
        compiled_scene,
        effective_plan,
        m_cache,
        worker_count,
        context,
        prefetcher,
        nullptr,
        compiled_scene,
        fps,
        rendered_frames,
        nullptr,  // no progress callback in this overload
        nullptr,   // no cancel flag
        m_disk_cache.get(),
        sink.get(), // Pass sink for streaming mode
        &result);   // Pass result for streaming stats

    // Copy rendered frames to result (buffered mode)
    if (!streaming_mode) {
        result.frames = std::move(rendered_frames);
    }

    // Finish output
    if (sink) {
        if (result.output_error.empty() && !sink->finish()) {
            result.output_error = sink->last_error();
        }
    }

    auto encode_end = std::chrono::high_resolution_clock::now();
    result.encode_time_ms = std::chrono::duration<double, std::milli>(encode_end - encode_start).count();

    // Audio Export and Muxing
    if (!compiled_scene.compositions.empty() && !compiled_scene.compositions.front().audio_tracks.empty()) {
        audio::AudioExporter exporter;
        for (const auto& track : compiled_scene.compositions.front().audio_tracks) {
            exporter.add_track(track);
        }

        audio::AudioExportConfig audio_config;
        std::filesystem::path audio_path = output_path.parent_path() / (output_path.stem().string() + ".wav");
        exporter.export_to(audio_path, audio_config);

        // Mux audio into video using FFmpeg
        if (!result.output_error.empty()) {
            // Skip muxing if video output failed
        } else if (std::filesystem::exists(audio_path) && std::filesystem::exists(output_path)) {
            std::string mux_error;
            if (!mux_audio_video(output_path.string(), audio_path.string(), mux_error)) {
                result.output_error = "Audio mux failed: " + mux_error;
            } else {
                // Clean up temporary WAV file after successful mux
                std::error_code ec;
                std::filesystem::remove(audio_path, ec);
            }
        }
    }

    // Calculate total wall time
    auto session_end = std::chrono::high_resolution_clock::now();
    result.wall_time_total_ms = std::chrono::duration<double, std::milli>(session_end - session_start).count();

    // Populate result metadata
    result.frame_count = result.frames_written; // Use frames_written in streaming mode
    result.width = static_cast<int>(execution_plan.render_plan.composition.width);
    result.height = static_cast<int>(execution_plan.render_plan.composition.height);
    result.fps_target = fps;
    result.thread_count = worker_count;

    if (result.frame_count > 0) {
        result.wall_time_per_frame_ms = result.wall_time_total_ms / result.frame_count;
    }

    if (result.cache_hits + result.cache_misses > 0) {
        result.cache_hit_rate = static_cast<double>(result.cache_hits) / (result.cache_hits + result.cache_misses);
    }

    // Scene metadata
    if (!compiled_scene.compositions.empty()) {
        result.composition_id = std::to_string(compiled_scene.compositions.front().node.node_id);
        result.duration_seconds = compiled_scene.compositions.front().duration;
    }

    // Generate ISO 8601 timestamp
    auto now = std::chrono::system_clock::now();
    auto now_time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::gmtime(&now_time_t), "%Y-%m-%dT%H:%M:%SZ");
    result.timestamp_iso8601 = ss.str();

    return result;
}

RenderSessionResult RenderSession::render(
    const SceneSpec& scene,
    const CompiledScene& compiled_scene,
    const RenderExecutionPlan& execution_plan,
    const std::filesystem::path& output_path) {

    return render(scene, compiled_scene, execution_plan, output_path, std::thread::hardware_concurrency());
}

RenderSessionResult RenderSession::render(
    const SceneSpec& scene,
    const CompiledScene& compiled_scene,
    const RenderExecutionPlan& execution_plan,
    const std::filesystem::path& output_path,
    std::size_t worker_count,
    RenderProgressCallback progress_callback) {

    // Create a dummy cancel flag (never set)
    CancelFlag dummy_cancel(false);
    return render(scene, compiled_scene, execution_plan, output_path, worker_count, progress_callback, dummy_cancel);
}

RenderSessionResult RenderSession::render(
    const SceneSpec& scene,
    const CompiledScene& compiled_scene,
    const RenderExecutionPlan& execution_plan,
    const std::filesystem::path& output_path,
    std::size_t worker_count,
    RenderProgressCallback progress_callback,
    CancelFlag& cancel_flag) {

    (void)scene;
    RenderSessionResult result;

    // Start timing the entire render session
    auto session_start = std::chrono::high_resolution_clock::now();

    RenderExecutionPlan effective_plan = execution_plan;
    const std::string resolved_output_path = output_path_or_plan(output_path, execution_plan.render_plan);
    if (!resolved_output_path.empty()) {
        effective_plan.render_plan.output.destination.path = resolved_output_path;
    }

    ::tachyon::RenderContext context(m_precomp_cache);
    context.renderer2d.font_registry = ::tachyon::renderer2d::get_default_font_registry();
    std::uint32_t w = static_cast<std::uint32_t>(execution_plan.render_plan.composition.width);
    std::uint32_t h = static_cast<std::uint32_t>(execution_plan.render_plan.composition.height);
    m_surface_pool = std::make_unique<runtime::RuntimeSurfacePool>(w, h, 10);
    context.surface_pool = m_surface_pool.get();

    media::MediaPrefetcher prefetcher;

    double fps = execution_plan.render_plan.composition.frame_rate.value();
    if (fps <= 0.0) fps = 24.0;

    // Create output sink and begin streaming mode
    auto encode_start = std::chrono::high_resolution_clock::now();
    std::unique_ptr<output::FrameOutputSink> sink = output::create_frame_output_sink(effective_plan.render_plan);
    if (sink) {
        if (!sink->begin(effective_plan.render_plan)) {
            result.output_error = sink->last_error();
            return result;
        }
        result.output_configured = true;
    }

    // Check if cancelled before starting
    if (cancel_flag.load()) {
        result.output_error = "Render cancelled by user";
        return result;
    }

    // Render frames with streaming to sink (memory-efficient)
    std::vector<ExecutedFrame> rendered_frames; // Kept for metadata compatibility
    render_frames_parallel(
        compiled_scene,
        effective_plan,
        m_cache,
        worker_count,
        context,
        prefetcher,
        nullptr,
        compiled_scene,
        fps,
        rendered_frames,
        progress_callback,
        &cancel_flag,
        m_disk_cache.get(),
        sink.get(), // Pass sink for streaming mode
        &result);   // Pass result for streaming stats

    // Check if cancelled during render
    if (cancel_flag.load()) {
        result.output_error = "Render cancelled by user";
        return result;
    }

    // Finish output
    if (sink) {
        if (result.output_error.empty() && !sink->finish()) {
            result.output_error = sink->last_error();
        }
    }

    auto encode_end = std::chrono::high_resolution_clock::now();
    result.encode_time_ms = std::chrono::duration<double, std::milli>(encode_end - encode_start).count();

    // Audio Export and Muxing
    if (!compiled_scene.compositions.empty() && !compiled_scene.compositions.front().audio_tracks.empty()) {
        audio::AudioExporter exporter;
        for (const auto& track : compiled_scene.compositions.front().audio_tracks) {
            exporter.add_track(track);
        }

        audio::AudioExportConfig audio_config;
        std::filesystem::path audio_path = output_path.parent_path() / (output_path.stem().string() + ".wav");
        exporter.export_to(audio_path, audio_config);

        // Mux audio into video using FFmpeg
        if (!result.output_error.empty()) {
            // Skip muxing if video output failed
        } else if (std::filesystem::exists(audio_path) && std::filesystem::exists(output_path)) {
            std::string mux_error;
            if (!mux_audio_video(output_path.string(), audio_path.string(), mux_error)) {
                result.output_error = "Audio mux failed: " + mux_error;
            } else {
                // Clean up temporary WAV file after successful mux
                std::error_code ec;
                std::filesystem::remove(audio_path, ec);
            }
        }
    }

    // Calculate total wall time
    auto session_end = std::chrono::high_resolution_clock::now();
    result.wall_time_total_ms = std::chrono::duration<double, std::milli>(session_end - session_start).count();

    // Populate result metadata
    result.frame_count = result.frames_written; // Use frames_written in streaming mode
    result.width = static_cast<int>(execution_plan.render_plan.composition.width);
    result.height = static_cast<int>(execution_plan.render_plan.composition.height);
    result.fps_target = fps;
    result.thread_count = worker_count;

    if (result.frame_count > 0) {
        result.wall_time_per_frame_ms = result.wall_time_total_ms / result.frame_count;
    }

    if (result.cache_hits + result.cache_misses > 0) {
        result.cache_hit_rate = static_cast<double>(result.cache_hits) / (result.cache_hits + result.cache_misses);
    }

    // Scene metadata
    if (!compiled_scene.compositions.empty()) {
        result.composition_id = std::to_string(compiled_scene.compositions.front().node.node_id);
        result.duration_seconds = compiled_scene.compositions.front().duration;
    }

    // Generate ISO 8601 timestamp
    auto now = std::chrono::system_clock::now();
    auto now_time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::gmtime(&now_time_t), "%Y-%m-%dT%H:%M:%SZ");
    result.timestamp_iso8601 = ss.str();

    return result;
}

} // namespace tachyon
