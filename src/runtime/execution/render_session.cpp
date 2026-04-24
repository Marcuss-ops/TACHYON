#include "tachyon/runtime/execution/session/render_session.h"
#include "tachyon/runtime/execution/frames/frame_executor.h"
#include "tachyon/runtime/execution/planning/render_plan.h"
#include "tachyon/runtime/cache/frame_cache.h"
#include "tachyon/runtime/resource/render_context.h"
#include "tachyon/runtime/resource/runtime_surface_pool.h"
#include "tachyon/renderer2d/resource/precomp_cache.h"
#include "tachyon/renderer2d/resource/texture_resolver.h"
#include "tachyon/output/frame_output_sink.h"
#include "tachyon/media/streaming/media_prefetcher.h"
#include "tachyon/audio/audio_export.h"

#include <iostream>
#include <future>
#include <atomic>
#include <algorithm>
#include <thread>

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
    std::vector<ExecutedFrame>& rendered_frames) {

    (void)original_scene;
    (void)session_fps;
    (void)scheduler;
    (void)prefetcher;

    const std::size_t task_count = execution_plan.frame_tasks.size();
    rendered_frames.resize(task_count);
    const std::size_t thread_count = std::max<std::size_t>(1, std::min(worker_count, task_count));
    std::atomic<std::size_t> next_index{0};
    std::vector<std::future<void>> workers;
    workers.reserve(thread_count);

    for (std::size_t worker_index = 0; worker_index < thread_count; ++worker_index) {
        workers.push_back(std::async(std::launch::async, [&]() {
            FrameArena arena;
            FrameExecutor executor(arena, cache, nullptr);
            
            // Create a thread-local context
            ::tachyon::RenderContext local_context(context.renderer2d.precomp_cache);
            local_context.media = context.media;
            local_context.ray_tracer = context.ray_tracer;
            local_context.policy = context.policy;
            local_context.surface_pool = context.surface_pool;
            local_context.renderer2d.font_registry = context.renderer2d.font_registry;
            
            for (;;) {
                const std::size_t index = next_index.fetch_add(1);
                if (index >= task_count) {
                    return;
                }

                const auto& task = execution_plan.frame_tasks[index];
                RenderPlan frame_plan = execution_plan.render_plan;
                
                DataSnapshot snapshot;
                rendered_frames[index] = executor.execute(compiled_scene, frame_plan, task, snapshot, local_context);
            }
        }));
    }

    for (auto& w : workers) {
        w.wait();
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
    
    double fps = execution_plan.render_plan.composition.frame_rate.value();
    if (fps <= 0.0) fps = 24.0;

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
        rendered_frames);

    result.frames = std::move(rendered_frames);
    for (const auto& frame : result.frames) {
        if (frame.cache_hit) {
            ++result.cache_hits;
        } else {
            ++result.cache_misses;
        }
    }

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
    }

    // Audio Export
    if (!compiled_scene.compositions.empty() && !compiled_scene.compositions.front().audio_tracks.empty()) {
        audio::AudioExporter exporter;
        for (const auto& track : compiled_scene.compositions.front().audio_tracks) {
            exporter.add_track(track);
        }
        
        audio::AudioExportConfig audio_config;
        std::filesystem::path audio_path = output_path.parent_path() / (output_path.stem().string() + ".wav");
        exporter.export_to(audio_path, audio_config);
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
