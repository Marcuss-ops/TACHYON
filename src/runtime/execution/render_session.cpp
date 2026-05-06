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
#include "tachyon/runtime/profiling/render_profiler.h"
#include "tachyon/media/management/asset_resolver.h"
#include "tachyon/runtime/telemetry/process_resource_sampler.h"

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
    std::size_t worker_count,
    CancelFlag* cancel_flag) {

    (void)scene;
    const auto session_start = std::chrono::steady_clock::now();
    
    ProcessResourceSampler sampler;
    sampler.start();

    RenderSessionResult result;

    RenderExecutionPlan effective_plan = execution_plan;
    const std::string resolved_output_path = output_path_or_plan(output_path, execution_plan.render_plan);
    if (!resolved_output_path.empty()) {
        effective_plan.render_plan.output.destination.path = resolved_output_path;
    }

    ::tachyon::RenderContext context(m_precomp_cache);
    context.policy = effective_plan.render_plan.quality_policy;
    
    // Initialize Asset Resolver
    media::AssetResolver::Config resolver_config;
    // TODO: Populate resolver_config from environment or scene spec if needed
    
    context.renderer2d.font_registry = ::tachyon::renderer2d::get_default_font_registry();
    
    auto resolver = std::make_shared<media::AssetResolver>(
        resolver_config,
        nullptr, // MediaManager is not an AssetManager
        context.media ? &context.media->image_manager() : nullptr,
        const_cast<text::FontRegistry*>(context.renderer2d.font_registry)
    );
    context.renderer2d.asset_resolver = resolver;
    std::uint32_t w = static_cast<std::uint32_t>(execution_plan.render_plan.composition.width);
    std::uint32_t h = static_cast<std::uint32_t>(execution_plan.render_plan.composition.height);
    m_surface_pool = std::make_unique<runtime::RuntimeSurfacePool>(w, h, 10);
    context.surface_pool = m_surface_pool.get();
    context.profiler = m_profiler;
    context.renderer2d.profiler = m_profiler;

    // Create and initialize output sink early for streaming mode
    std::unique_ptr<output::FrameOutputSink> sink = output::create_frame_output_sink(effective_plan.render_plan);
    
    // Determine audio export path early if needed
    std::filesystem::path audio_export_path;
    bool is_temp_audio = false;
    if (sink && audio::has_any_audio(effective_plan.render_plan)) {
        const bool is_video = output::output_requests_video_file(effective_plan.render_plan.output);
        const std::string final_output = effective_plan.render_plan.output.destination.path;
        std::filesystem::path final_p(final_output);

        if (is_video) {
#ifdef TACHYON_ENABLE_AUDIO_MUX
            audio_export_path = final_p.parent_path() / (final_p.stem().string() + ".temp.wav");
            is_temp_audio = true;
            sink->set_audio_source(audio_export_path.string());
#endif
        } else {
            audio_export_path = final_p.parent_path() / (final_p.stem().string() + ".wav");
        }
    }

    if (sink) {
        sink->set_profiler(m_profiler);
        profiling::ProfileScope scope(m_profiler, profiling::ProfileEventType::Phase, "initialize_sink");
        if (!sink->begin(effective_plan.render_plan)) {
            result.output_error = sink->last_error();
            
            sampler.stop();
            const auto session_end = std::chrono::steady_clock::now();
            result.wall_time_total_ms = std::chrono::duration<double, std::milli>(session_end - session_start).count();
            
            result.peak_working_set_bytes = sampler.peak_working_set_bytes();
            result.avg_working_set_bytes = sampler.avg_working_set_bytes();
            result.peak_private_bytes = sampler.peak_private_bytes();
            result.avg_private_bytes = sampler.avg_private_bytes();
            result.avg_cpu_percent_machine = sampler.avg_cpu_percent_machine();
            result.avg_cpu_cores_used = sampler.avg_cpu_cores_used();
            return result;
        }
        result.output_configured = true;
    }

    media::MediaPrefetcher prefetcher;
    std::vector<double> frame_times;
    
    double fps = execution_plan.render_plan.composition.frame_rate.value();
    if (fps <= 0.0) fps = 24.0;

    const auto frame_exec_start = std::chrono::high_resolution_clock::now();
    frame_times.resize(execution_plan.frame_tasks.size());
    

    std::vector<ExecutedFrame> rendered_frames;

    {
        profiling::ProfileScope scope(m_profiler, profiling::ProfileEventType::Phase, "render_frames_loop");
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
            cancel_flag,
            nullptr,
            sink.get(),
            &result,
            &frame_times);
    }

    if (!sink) {
        result.frames = std::move(rendered_frames);
        result.cache_hits = 0;
        result.cache_misses = 0;
        for (const auto& frame : result.frames) {
            if (frame.cache_hit) ++result.cache_hits;
            else ++result.cache_misses;
        }
    }
    const auto frame_exec_end = std::chrono::high_resolution_clock::now();
    result.frame_execution_ms = std::chrono::duration<double, std::milli>(frame_exec_end - frame_exec_start).count();
    result.frame_times_ms = std::move(frame_times);

    // Streaming mode handles frame writing during render, no post-render loop needed

    // Audio Export
    if (!audio_export_path.empty()) {
        profiling::ProfileScope scope(m_profiler, profiling::ProfileEventType::AudioMux, "audio_export");
        audio::export_plan_audio(effective_plan.render_plan, audio_export_path, cancel_flag);
    }

    // Finalize sink after all frames are rendered
    if (sink) {
        profiling::ProfileScope scope(m_profiler, profiling::ProfileEventType::Phase, "finalize_sink");
        const auto encode_start = std::chrono::high_resolution_clock::now();
        if (result.output_error.empty() && !sink->finish()) {
            result.output_error = sink->last_error();
        }
        const auto encode_end = std::chrono::high_resolution_clock::now();
        result.encode_ms = std::chrono::duration<double, std::milli>(encode_end - encode_start).count();
    }

    // Cleanup temp audio
    if (!audio_export_path.empty() && is_temp_audio) {
        std::error_code ec;
        std::filesystem::remove(audio_export_path, ec);
    }

    sampler.stop();
    const auto session_end = std::chrono::steady_clock::now();
    result.wall_time_total_ms = std::chrono::duration<double, std::milli>(session_end - session_start).count();
    
    result.peak_working_set_bytes = sampler.peak_working_set_bytes();
    result.avg_working_set_bytes = sampler.avg_working_set_bytes();
    result.peak_private_bytes = sampler.peak_private_bytes();
    result.avg_private_bytes = sampler.avg_private_bytes();
    result.avg_cpu_percent_machine = sampler.avg_cpu_percent_machine();
    result.avg_cpu_cores_used = sampler.avg_cpu_cores_used();

    const std::size_t total_frames = result.frames_written > 0 ? result.frames_written : effective_plan.frame_tasks.size();
    if (total_frames > 0) {
        result.wall_time_per_frame_ms = result.wall_time_total_ms / static_cast<double>(total_frames);
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
