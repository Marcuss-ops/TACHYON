#include "tachyon/runtime/execution/session/render_session.h"
#include "tachyon/runtime/execution/frames/frame_executor.h"
#include "tachyon/runtime/execution/planning/render_plan.h"
#include "tachyon/runtime/cache/frame_cache.h"
#include "tachyon/runtime/resource/render_context.h"
#include "tachyon/runtime/resource/runtime_surface_pool.h"
#include "tachyon/renderer2d/resource/precomp_cache.h"
#include "tachyon/media/streaming/media_prefetcher.h"
#include "tachyon/audio/audio_export.h"

#include <iostream>
#include <future>
#include <atomic>
#include <algorithm>
#include <thread>

namespace tachyon {

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
            FrameExecutor executor(arena, cache, context.surface_pool);
            
            // Create a thread-local context
            ::tachyon::RenderContext local_context(context.renderer2d.precomp_cache);
            local_context.media = context.media;
            local_context.ray_tracer = context.ray_tracer;
            local_context.policy = context.policy;
            local_context.surface_pool = context.surface_pool;
            
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
    
    FrameCache cache;
    auto precomp_cache = std::make_shared<renderer2d::PrecompCache>();
    ::tachyon::RenderContext context(precomp_cache);
    
    std::uint32_t w = static_cast<std::uint32_t>(execution_plan.render_plan.composition.width);
    std::uint32_t h = static_cast<std::uint32_t>(execution_plan.render_plan.composition.height);
    runtime::RuntimeSurfacePool surface_pool(w, h, 10);
    context.surface_pool = &surface_pool;
    
    media::MediaPrefetcher prefetcher;
    std::vector<ExecutedFrame> rendered_frames;
    
    double fps = execution_plan.render_plan.composition.frame_rate.value();
    if (fps <= 0.0) fps = 24.0;

    render_frames_parallel(
        compiled_scene,
        execution_plan,
        cache,
        worker_count,
        context,
        prefetcher,
        nullptr,
        compiled_scene,
        fps,
        rendered_frames);

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

    result.output_configured = true;
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
