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
#include "tachyon/output/output_planner.h"
#include "tachyon/media/streaming/media_prefetcher.h"
#include "tachyon/audio/audio_export.h"
#include "tachyon/media/resolution/asset_path_utils.h"
#include "tachyon/runtime/execution/session/render_internal.h"
#include "tachyon/runtime/profiling/render_profiler.h"
#include "tachyon/media/management/asset_resolver.h"
#include "tachyon/runtime/telemetry/process_resource_sampler.h"
#include "tachyon/runtime/policy/worker_policy.h"
#include "tachyon/runtime/policy/surface_pool_policy.h"
#include "tachyon/runtime/policy/telemetry_policy.h"

#include <iostream>
#include <future>
#include <atomic>
#include <algorithm>
#include <thread>
#include <chrono>
#include <utility>

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

class ScopedProcessSampler {
public:
    explicit ScopedProcessSampler(bool enabled) : enabled_(enabled) {
        if (enabled_) {
            sampler_.start();
        }
    }

    void stop() {
        if (enabled_) {
            sampler_.stop();
        }
    }

    bool enabled() const { return enabled_; }
    ProcessResourceSampler& sampler() { return sampler_; }

    std::size_t peak_working_set_bytes() const { return enabled_ ? sampler_.peak_working_set_bytes() : 0; }
    std::size_t avg_working_set_bytes() const { return enabled_ ? sampler_.avg_working_set_bytes() : 0; }
    std::size_t peak_private_bytes() const { return enabled_ ? sampler_.peak_private_bytes() : 0; }
    std::size_t avg_private_bytes() const { return enabled_ ? sampler_.avg_private_bytes() : 0; }
    double avg_cpu_percent_machine() const { return enabled_ ? sampler_.avg_cpu_percent_machine() : 0.0; }
    double avg_cpu_cores_used() const { return enabled_ ? sampler_.avg_cpu_cores_used() : 0.0; }

private:
    bool enabled_{false};
    ProcessResourceSampler sampler_;
};

media::AssetResolver::Config build_asset_resolver_config(const RenderExecutionPlan& plan) {
    return media::AssetResolver::Config(media::ProjectResolutionContext::from_scene(plan.render_plan.scene_ref));
}

struct RenderSessionWorkspace {
    RenderExecutionPlan effective_plan;
    std::string resolved_output_path;
    ::tachyon::RenderContext context;
    std::unique_ptr<output::FrameOutputSink> sink;
    output::AudioExportPlan audio_plan;
    media::MediaPrefetcher& prefetcher;
    std::vector<double> frame_times;
    std::vector<ExecutedFrame> rendered_frames;

    RenderSessionWorkspace(
        std::shared_ptr<renderer2d::PrecompCache> precomp_cache,
        const RenderExecutionPlan& execution_plan,
        const std::filesystem::path& output_path,
        media::MediaPrefetcher& prefetcher_ref)
        : effective_plan(execution_plan),
          resolved_output_path(output_path_or_plan(output_path, execution_plan.render_plan)),
          context(std::move(precomp_cache)),
          prefetcher(prefetcher_ref) {}
};

void configure_render_context(
    RenderSessionWorkspace& workspace,
    profiling::RenderProfiler* profiler,
    runtime::RuntimeSurfacePool* surface_pool,
    const renderer2d::EffectRegistry& effect_registry,
    const renderer3d::Modifier3DRegistry& modifier_registry,
    const TransitionRegistry& transition_registry) {
    workspace.context.policy = workspace.effective_plan.render_plan.quality_policy;
    workspace.context.renderer2d.font_registry = ::tachyon::renderer2d::get_default_font_registry();

    const auto resolver_config = build_asset_resolver_config(workspace.effective_plan);
    workspace.context.renderer2d.asset_resolver = std::make_shared<media::AssetResolver>(
        resolver_config,
        nullptr,
        workspace.context.media ? &workspace.context.media->image_manager() : nullptr,
        const_cast<text::FontRegistry*>(workspace.context.renderer2d.font_registry));

    workspace.context.surface_pool = surface_pool;
    workspace.context.profiler = profiler;
    workspace.context.renderer2d.profiler = profiler;
    workspace.context.renderer2d.effects = renderer2d::create_effect_host(effect_registry);
    workspace.context.renderer2d.modifier_registry = &modifier_registry;
    workspace.context.renderer2d.transition_registry = &transition_registry;
}

bool begin_output_sink(
    RenderSessionWorkspace& workspace,
    RenderSessionResult& result,
    profiling::RenderProfiler* profiler,
    ScopedProcessSampler& sampler,
    const std::chrono::steady_clock::time_point& session_start) {
    if (!workspace.sink) {
        return true;
    }

    workspace.sink->set_profiler(profiler);
    profiling::ProfileScope scope(profiler, profiling::ProfileEventType::Phase, "initialize_sink");
    if (workspace.sink->begin(workspace.effective_plan.render_plan)) {
        result.output_configured = true;
        return true;
    }

    result.output_error = workspace.sink->last_error();
    sampler.stop();
    const auto session_end = std::chrono::steady_clock::now();
    result.wall_time_total_ms = std::chrono::duration<double, std::milli>(session_end - session_start).count();
    result.peak_working_set_bytes = sampler.peak_working_set_bytes();
    result.avg_working_set_bytes = sampler.avg_working_set_bytes();
    result.peak_private_bytes = sampler.peak_private_bytes();
    result.avg_private_bytes = sampler.avg_private_bytes();
    result.avg_cpu_percent_machine = sampler.avg_cpu_percent_machine();
    result.avg_cpu_cores_used = sampler.avg_cpu_cores_used();
    return false;
}

void execute_render_loop(
    RenderSessionWorkspace& workspace,
    const CompiledScene& compiled_scene,
    FrameCache& cache,
    std::size_t worker_count,
    CancelFlag* cancel_flag,
    RenderSessionResult* result) {
    profiling::ProfileScope scope(
        workspace.context.profiler,
        profiling::ProfileEventType::Phase,
        "render_frames_loop");
    render_frames_parallel_internal(
        compiled_scene,
        workspace.effective_plan,
        cache,
        worker_count,
        workspace.context,
        workspace.prefetcher,
        nullptr,
        workspace.rendered_frames,
        nullptr,
        cancel_flag,
        nullptr,
        workspace.sink.get(),
        result,
        &workspace.frame_times);
}

void finalize_render_output(
    RenderSessionWorkspace& workspace,
    RenderSessionResult& result,
    CancelFlag* cancel_flag) {
    if (!workspace.audio_plan.path.empty()) {
        profiling::ProfileScope scope(
            workspace.context.profiler,
            profiling::ProfileEventType::AudioMux,
            "audio_export");
        audio::export_plan_audio(workspace.effective_plan.render_plan, workspace.audio_plan.path, cancel_flag);
    }

    if (workspace.sink) {
        profiling::ProfileScope scope(
            workspace.context.profiler,
            profiling::ProfileEventType::Phase,
            "finalize_sink");
        const auto encode_start = std::chrono::high_resolution_clock::now();
        if (result.output_error.empty() && !workspace.sink->finish()) {
            result.output_error = workspace.sink->last_error();
        }
        const auto encode_end = std::chrono::high_resolution_clock::now();
        result.encode_ms = std::chrono::duration<double, std::milli>(encode_end - encode_start).count();
    }

    if (!workspace.audio_plan.path.empty() && workspace.audio_plan.is_temporary) {
        std::error_code ec;
        std::filesystem::remove(workspace.audio_plan.path, ec);
    }
}

void finalize_session_metrics(
    RenderSessionResult& result,
    ScopedProcessSampler& sampler,
    const std::chrono::steady_clock::time_point& session_start,
    const std::chrono::steady_clock::time_point& session_end) {
    sampler.stop();
    result.wall_time_total_ms = std::chrono::duration<double, std::milli>(session_end - session_start).count();
    result.peak_working_set_bytes = sampler.peak_working_set_bytes();
    result.avg_working_set_bytes = sampler.avg_working_set_bytes();
    result.peak_private_bytes = sampler.peak_private_bytes();
    result.avg_private_bytes = sampler.avg_private_bytes();
    result.avg_cpu_percent_machine = sampler.avg_cpu_percent_machine();
    result.avg_cpu_cores_used = sampler.avg_cpu_cores_used();

    const std::size_t total_frames = result.frames_written > 0 ? result.frames_written : result.frames.size();
    if (total_frames > 0) {
        result.wall_time_per_frame_ms = result.wall_time_total_ms / static_cast<double>(total_frames);
    }
}

} // namespace


RenderSession::RenderSession() {
    renderer2d::register_builtin_effects(m_effect_registry, m_transition_registry);
    renderer3d::register_builtin_modifiers(m_modifier_registry);
    register_builtin_transitions(m_transition_registry);
}

RenderSessionResult RenderSession::render(
    const SceneSpec& scene,
    const CompiledScene& compiled_scene,
    const RenderExecutionPlan& execution_plan,
    const std::filesystem::path& output_path,
    std::size_t worker_count,
    CancelFlag* cancel_flag) {

    (void)scene;
    const auto session_start = std::chrono::steady_clock::now();
    
    runtime::TelemetryPolicy telemetry_policy;
    ScopedProcessSampler sampler(telemetry_policy.should_sample_process());

    RenderSessionResult result;
    RenderSessionWorkspace workspace(m_precomp_cache, execution_plan, output_path, m_prefetcher);
    if (!workspace.resolved_output_path.empty()) {
        workspace.effective_plan.render_plan.output.destination.path = workspace.resolved_output_path;
    }

    std::uint32_t w = static_cast<std::uint32_t>(workspace.effective_plan.render_plan.composition.width);
    std::uint32_t h = static_cast<std::uint32_t>(workspace.effective_plan.render_plan.composition.height);
    
    runtime::SurfacePoolPolicy surface_policy;
    const auto surface_count = surface_policy.resolve(w, h, worker_count);
    m_surface_pool = std::make_unique<runtime::RuntimeSurfacePool>(w, h, surface_count);
    
    configure_render_context(workspace, m_profiler, m_surface_pool.get(), m_effect_registry, m_modifier_registry, m_transition_registry);

    workspace.sink = output::create_frame_output_sink(workspace.effective_plan.render_plan);
    
    const bool is_video = output::output_requests_video_file(workspace.effective_plan.render_plan.output);
    workspace.audio_plan = output::plan_audio_export(workspace.effective_plan.render_plan, is_video);
    
    if (workspace.sink && !workspace.audio_plan.path.empty() && workspace.audio_plan.is_temporary) {
        workspace.sink->set_audio_source(workspace.audio_plan.path.string());
    }

    if (!begin_output_sink(workspace, result, m_profiler, sampler, session_start)) {
        return result;
    }

    workspace.frame_times.resize(workspace.effective_plan.frame_tasks.size());
    const auto frame_exec_start = std::chrono::high_resolution_clock::now();
    execute_render_loop(workspace, compiled_scene, m_cache, worker_count, cancel_flag, &result);
    const auto frame_exec_end = std::chrono::high_resolution_clock::now();
    result.frame_execution_ms = std::chrono::duration<double, std::milli>(frame_exec_end - frame_exec_start).count();
    result.frame_times_ms = std::move(workspace.frame_times);

    if (!workspace.sink) {
        result.frames = std::move(workspace.rendered_frames);
        result.cache_hits = 0;
        result.cache_misses = 0;
        for (const auto& frame : result.frames) {
            if (frame.cache_hit) {
                ++result.cache_hits;
            } else {
                ++result.cache_misses;
            }
        }
    }

    finalize_render_output(workspace, result, cancel_flag);

    const auto session_end = std::chrono::steady_clock::now();
    finalize_session_metrics(result, sampler, session_start, session_end);

    return result;
}

RenderSessionResult RenderSession::render(
    const SceneSpec& scene,
    const CompiledScene& compiled_scene,
    const RenderExecutionPlan& execution_plan,
    const std::filesystem::path& output_path) {
    
    runtime::WorkerPolicy policy;
    return render(scene, compiled_scene, execution_plan, output_path, 
                  policy.resolve(std::thread::hardware_concurrency()));
}

} // namespace tachyon
