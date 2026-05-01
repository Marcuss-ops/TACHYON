#include "tachyon/runtime/execution/session/render_session.h"
#include "tachyon/runtime/execution/frames/frame_executor.h"
#include "tachyon/runtime/execution/planning/render_plan.h"
#include "tachyon/runtime/cache/frame_cache.h"
#include "tachyon/runtime/resource/render_context.h"
#include "tachyon/runtime/resource/runtime_surface_pool.h"
#include "tachyon/renderer2d/resource/precomp_cache.h"
#include "tachyon/renderer2d/resource/texture_resolver.h"
#include "tachyon/output/frame_output_sink.h"
#include "tachyon/output/output_utils.h"
#include "tachyon/media/streaming/media_prefetcher.h"
#include "tachyon/audio/audio_export.h"
#include "tachyon/runtime/execution/session/render_internal.h"

#include <iostream>
#include <future>
#include <atomic>
#include <algorithm>
#include <thread>
#include <chrono>

namespace tachyon {

namespace {

output::OutputFramePacket make_output_packet(const ExecutedFrame& frame) {
    output::OutputFramePacket packet;
    packet.frame_number = frame.frame_number;
    packet.frame = frame.frame.get();
    packet.metadata.time_seconds = static_cast<double>(frame.frame_number);
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

} // namespace


RenderSessionResult RenderSession::render(
    const SceneSpec& scene,
    const CompiledScene& compiled_scene,
    const RenderExecutionPlan& execution_plan,
    const std::filesystem::path& output_path,
    std::size_t worker_count) {

    (void)scene;
    RenderSessionResult result;

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
    std::vector<ExecutedFrame> rendered_frames;
    std::vector<double> frame_times;
    
    double fps = execution_plan.render_plan.composition.frame_rate.value();
    if (fps <= 0.0) fps = 24.0;

    const auto frame_exec_start = std::chrono::high_resolution_clock::now();
    frame_times.resize(execution_plan.frame_tasks.size());
    render_frames_parallel_internal(
        compiled_scene,
        effective_plan,
        m_cache,
        worker_count,
        context,
        prefetcher,
        nullptr,
        rendered_frames,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        &result,
        &frame_times);
    const auto frame_exec_end = std::chrono::high_resolution_clock::now();
    result.frame_execution_ms = std::chrono::duration<double, std::milli>(frame_exec_end - frame_exec_start).count();
    result.frame_times_ms = std::move(frame_times);

    result.frames = std::move(rendered_frames);
    for (const auto& frame : result.frames) {
        if (frame.cache_hit) {
            ++result.cache_hits;
        } else {
            ++result.cache_misses;
        }
    }

    const auto encode_start = std::chrono::high_resolution_clock::now();
    std::unique_ptr<output::FrameOutputSink> sink = output::create_frame_output_sink(effective_plan.render_plan);
    if (sink) {
        if (!sink->begin(effective_plan.render_plan)) {
            result.output_error = sink->last_error();
            return result;
        }

        result.output_configured = true;
        for (const auto& frame : result.frames) {
            if (!frame.frame) {
                continue;
            }

            output::OutputFramePacket packet = make_output_packet(frame);
            if (!sink->write_frame(packet)) {
                result.output_error = sink->last_error();
                break;
            }
            ++result.frames_written;
        }

        if (result.output_error.empty() && !sink->finish()) {
            result.output_error = sink->last_error();
        }

        const auto encode_end = std::chrono::high_resolution_clock::now();
        result.encode_ms = std::chrono::duration<double, std::milli>(encode_end - encode_start).count();
    }

    // Audio Export (Standalone WAV if sink doesn't handle muxing)
    if (!compiled_scene.compositions.empty() && !compiled_scene.compositions.front().audio_tracks.empty()) {
        const bool sink_handles_audio = output::output_requests_video_file(effective_plan.render_plan.output);
        if (!sink_handles_audio) {
            audio::AudioExporter exporter;
            for (const auto& track : compiled_scene.compositions.front().audio_tracks) {
                exporter.add_track(track);
            }
            
            audio::AudioExportConfig audio_config;
            std::filesystem::path audio_path = output_path.parent_path() / (output_path.stem().string() + ".wav");
            exporter.export_to(audio_path, audio_config);
        }
    }

    return result;
}

RenderSessionResult RenderSession::render(
    const SceneSpec& scene,
    const CompiledScene& compiled_scene,
    const RenderExecutionPlan& execution_plan,
    const std::filesystem::path& output_path) {
    
    return render(scene, compiled_scene, execution_plan, output_path, std::thread::hardware_concurrency());
}

} // namespace tachyon
