#include "tachyon/runtime/execution/session/render_session.h"
#include "tachyon/runtime/execution/render_session_parallel.h"
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
#include "tachyon/audio/io/audio_export.h"

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

// Forward declarations for extracted serialization helpers
std::vector<uint8_t> serialize_framebuffer(const renderer2d::Framebuffer& fb);
std::shared_ptr<renderer2d::Framebuffer> deserialize_framebuffer(const std::vector<uint8_t>& data);

// Forward declaration for audio muxing helper
bool mux_audio_video(const std::string& video_path, const std::string& audio_path, std::string& error);

output::OutputFramePacket make_output_packet(const ExecutedFrame& frame, double fps, const renderer2d::ColorManagementSystem& cms) {
    output::OutputFramePacket packet;
    packet.frame_number = frame.frame_number;
    packet.frame = frame.frame.get();
    packet.metadata.time_seconds = frame.frame_number / fps;
    packet.metadata.scene_hash = std::to_string(frame.scene_hash);
    packet.metadata.color_space = cms.output_profile.to_string();
    return packet;
}

std::string output_path_or_plan(const std::filesystem::path& output_path, const RenderPlan& plan) {
    if (!output_path.empty()) {
        return output_path.string();
    }
    return plan.output.destination.path;
}

RenderSessionResult RenderSession::render(
    const SceneSpec& scene,
    const CompiledScene& compiled_scene,
    const RenderExecutionPlan& execution_plan,
    const std::filesystem::path& output_path,
    std::size_t worker_count) {

    (void)scene;
    RenderSessionResult result;

#ifdef TACHYON_TRACY_ENABLED
    ZoneScopedN("RenderSession::render");
#endif

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

