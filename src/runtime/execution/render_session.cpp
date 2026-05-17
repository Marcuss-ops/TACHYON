#include "tachyon/runtime/execution/session/render_session.h"
#include "tachyon/runtime/registry/engine_registry.h"
#include "tachyon/runtime/execution/frames/frame_executor.h"
#include "tachyon/runtime/execution/planning/render_plan.h"
#include "tachyon/runtime/cache/frame_cache.h"
#include "tachyon/runtime/resource/render_context.h"
#include "tachyon/runtime/resource/surface_pool.h"
#include "tachyon/renderer2d/effects/effect_host.h"
#include "tachyon/renderer2d/resource/precomp_cache.h"
#include "tachyon/renderer2d/resource/texture_resolver.h"
#include "tachyon/output/frame_output_sink.h"
#include "tachyon/output/output_utils.h"
#include "tachyon/output/output_planner.h"
#include "tachyon/core/media/media_interfaces.h"
#include "tachyon/core/media/media_provider.h"
#include "tachyon/core/media/asset_resolver_interface.h"
#include "tachyon/core/audio/audio_export_interface.h"
#include "tachyon/runtime/execution/session/render_internal.h"
#include "tachyon/runtime/profiling/render_profiler.h"
// Forward declarations or interfaces already included
#include "tachyon/renderer2d/effects/core/transitions/basic_transitions.h"
#include "tachyon/renderer2d/effects/core/transitions/artistic_transitions.h"
#include "tachyon/renderer2d/effects/core/transitions/light_leak_transitions.h"
#include "tachyon/runtime/telemetry/process_resource_sampler.h"
#include "tachyon/runtime/telemetry/telemetry_writer.h"
#include "tachyon/runtime/telemetry/render_telemetry_record.h"
#include "tachyon/runtime/policy/worker_policy.h"
#include "tachyon/runtime/policy/surface_pool_policy.h"
#include "tachyon/runtime/policy/telemetry_policy.h"
#include "tachyon/core/render_telemetry.h"
#include <iomanip>
#include <sstream>

#include <iostream>
#include <future>
#include <atomic>
#include <algorithm>
#include <thread>
#include <chrono>
#include <utility>

namespace tachyon {

namespace {


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

// Removed concrete AssetResolver usage

struct RenderSessionWorkspace {
    RenderExecutionPlan effective_plan;
    std::string resolved_output_path;
    ::tachyon::RenderContext context;
    std::unique_ptr<output::FrameOutputSink> sink;
    output::AudioExportPlan audio_plan;
#ifdef TACHYON_ENABLE_MEDIA
    media::IMediaPrefetcher* prefetcher;
#endif
    std::vector<double> frame_times;
    std::vector<ExecutedFrame> rendered_frames;

    RenderSessionWorkspace(
        std::shared_ptr<renderer2d::PrecompCache> precomp_cache,
        std::shared_ptr<renderer2d::PrecompCache> text_surface_cache,
        const RenderExecutionPlan& execution_plan,
        const std::filesystem::path& output_path,
        media::IMediaPrefetcher* prefetcher_ptr)
        : effective_plan(execution_plan),
          resolved_output_path(output_path_or_plan(output_path, execution_plan.render_plan)),
          context(std::move(precomp_cache))
#ifdef TACHYON_ENABLE_MEDIA
          , prefetcher(prefetcher_ptr)
#endif
    {
        (void)prefetcher_ptr;
        context.text_surface_cache = std::move(text_surface_cache);
    }
};

void configure_render_context(
    RenderSessionWorkspace& workspace,
    profiling::RenderProfiler* profiler,
    std::shared_ptr<SurfacePool> surface_pool,
    const renderer2d::EffectRegistry& effect_registry,
    const TransitionRegistry& transition_registry,
    const presets::TextRegistry* text_registry,
    audio::IAudioExporter* audio_exporter) {
    workspace.context.policy = workspace.effective_plan.render_plan.quality_policy;
#ifdef TACHYON_ENABLE_TEXT
    workspace.context.font_registry = ::tachyon::renderer2d::get_default_font_registry();
#else
    workspace.context.font_registry = nullptr;
#endif

    // AssetResolver should be injected or handled by the caller who knows about Operations

    workspace.context.surface_pool = surface_pool;
    workspace.context.profiler = profiler;
    workspace.context.effects = renderer2d::create_effect_host(effect_registry);
    workspace.context.transition_registry = &transition_registry;
    workspace.context.text_registry = text_registry;
#ifdef TACHYON_ENABLE_MEDIA
    workspace.context.audio_exporter = audio_exporter;
#endif
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
    const ::tachyon::runtime::RenderWorkerBudget& budget,
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
        budget,
        workspace.context,
#ifdef TACHYON_ENABLE_MEDIA
        workspace.prefetcher,
#else
        nullptr,
#endif
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
#ifdef TACHYON_ENABLE_MEDIA
        if (workspace.context.audio_exporter) {
            workspace.context.audio_exporter->export_plan_audio(workspace.effective_plan.render_plan, workspace.audio_plan.path, cancel_flag);
        }
#endif
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
    m_bundle = std::make_unique<runtime::EngineRegistry>(runtime::create_default_runtime_registry_bundle());
}

void RenderSession::set_registry_bundle(const runtime::EngineRegistry* bundle) {
    m_bundle_ptr = bundle;
}

RenderSessionResult RenderSession::render(
    const SceneSpec& scene,
    const CompiledScene& compiled_scene,
    const RenderExecutionPlan& execution_plan,
    const std::filesystem::path& output_path,
    const ::tachyon::runtime::RenderWorkerBudget& budget,
    CancelFlag* cancel_flag) {

    (void)scene;
    const auto session_start = std::chrono::steady_clock::now();
    
    runtime::TelemetryPolicy telemetry_policy;
    ScopedProcessSampler sampler(telemetry_policy.should_sample_process());

    RenderSessionResult result;
    RenderSessionWorkspace workspace(m_precomp_cache, m_text_surface_cache, execution_plan, output_path, m_prefetcher.get());
    if (!workspace.resolved_output_path.empty()) {
        workspace.effective_plan.render_plan.output.destination.path = workspace.resolved_output_path;
    }

    std::uint32_t w = static_cast<std::uint32_t>(workspace.effective_plan.render_plan.composition.width);
    std::uint32_t h = static_cast<std::uint32_t>(workspace.effective_plan.render_plan.composition.height);
    
    runtime::SurfacePoolPolicy surface_policy;
    m_surface_pool->set_policy(surface_policy);
    m_surface_pool->prepare(w, h, budget.frame_concurrency);

    const TransitionRegistry* transition_registry = m_transition_registry_ptr;
    const renderer2d::EffectRegistry* effect_registry = nullptr;
    const presets::TextRegistry* text_registry = m_text_registry_ptr;

    if (m_bundle_ptr) {
        if (!transition_registry) transition_registry = &m_bundle_ptr->transitions;
        effect_registry = &m_bundle_ptr->effects;
#ifdef TACHYON_ENABLE_TEXT
        if (!text_registry) text_registry = m_bundle_ptr->text_registry.get();
#endif
    } else if (m_bundle) {
        if (!transition_registry) transition_registry = &m_bundle->transitions;
        effect_registry = &m_bundle->effects;
#ifdef TACHYON_ENABLE_TEXT
        if (!text_registry) text_registry = m_bundle->text_registry.get();
#endif
    }
    
    if (!effect_registry || !transition_registry) {
        throw std::runtime_error("RenderSession: No registry bundle or registries provided");
    }

    configure_render_context(workspace, m_profiler, m_surface_pool, *effect_registry, *transition_registry, text_registry, m_audio_exporter);

    // Phase 0: Preflight (Asset/Font/Writability checks)
    if (!preflight(scene, compiled_scene, execution_plan, workspace.context, result)) {
        return result;
    }

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
    execute_render_loop(workspace, compiled_scene, m_cache, budget, cancel_flag, &result);
    const auto frame_exec_end = std::chrono::high_resolution_clock::now();
    result.frame_execution_ms = std::chrono::duration<double, std::milli>(frame_exec_end - frame_exec_start).count();
    result.frame_times_ms = std::move(workspace.frame_times);

    // New Structured Telemetry logging for each frame
    for (size_t i = 0; i < result.frame_times_ms.size(); ++i) {
        TelemetryEvent fe;
        fe.event = is_video ? "video_frame" : "frame_render";
        fe.frame = static_cast<int>(workspace.effective_plan.frame_tasks[i].frame_number);
        fe.w = w;
        fe.h = h;
        fe.total_ms = result.frame_times_ms[i];
        // Note: finer-grained timings (setup/composite/blur) are currently aggregated in frame_executor
        RenderTelemetry::get().log(fe);
    }

    // Always move the rendered frames (buffered or shell frames) to enable precise cache hits reporting
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

    finalize_render_output(workspace, result, cancel_flag);

    const auto session_end = std::chrono::steady_clock::now();
    finalize_session_metrics(result, sampler, session_start, session_end);

    // SQLite Telemetry Persistence
    {
        RenderTelemetryRecord record;
        
        // Identity
        record.run_id = "session-" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
        record.job_id = "job-" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
        record.scene_id = workspace.resolved_output_path.empty() ? "anonymous_scene" : std::filesystem::path(workspace.resolved_output_path).stem().string();
        record.preset_id = "default";
        record.machine_id = "local";

        // Status
        record.success = (result.output_error.empty() && result.diagnostics.ok());
        if (!record.success) {
            record.error_code = "RenderFailed";
            record.error_message = result.output_error.empty() ? "Diagnostics error" : result.output_error;
        }

        // Output stats
        record.frames_total = static_cast<int>(workspace.effective_plan.frame_tasks.size());
        record.frames_written = static_cast<int>(result.frames_written);
        record.duration_seconds = static_cast<double>(record.frames_total) / 30.0; // fallback standard 30fps
        record.fps_target = 30.0;

        // Timings (ms)
        record.wall_time_ms = result.wall_time_total_ms;
        record.render_ms = result.frame_execution_ms;
        record.encode_ms = result.encode_ms;
        record.io_read_ms = result.io_read_ms;
        record.io_write_ms = result.io_write_ms;
        record.setup_ms = result.scene_compile_ms + result.plan_build_ms + result.execution_plan_build_ms;

        // Performance Metrics
        if (record.wall_time_ms > 0.0) {
            record.effective_fps = (static_cast<double>(record.frames_written) / (record.wall_time_ms / 1000.0));
            record.videos_per_hour = 3600000.0 / record.wall_time_ms;
        }
        
        if (record.duration_seconds > 0.0 && record.wall_time_ms > 0.0) {
            record.video_seconds_per_render_second = record.duration_seconds / (record.wall_time_ms / 1000.0);
        }

        // Resources
        record.peak_working_set_bytes = result.peak_working_set_bytes;
        record.avg_working_set_bytes = result.avg_working_set_bytes;
        record.peak_private_bytes = result.peak_private_bytes;
        record.avg_private_bytes = result.avg_private_bytes;
        record.avg_cpu_percent_machine = result.avg_cpu_percent_machine;
        record.avg_cpu_cores_used = result.avg_cpu_cores_used;

        // I/O
        record.input_bytes = result.input_bytes;
        record.output_bytes = result.output_bytes;

        record.cache_hit_rate = result.cache_hit_rate();
        record.total_pixels_processed = result.total_pixels_processed;
        record.total_tiles = result.total_tiles;

        // Metadata
        record.output_path = workspace.resolved_output_path;
        
        // Finished Time
        auto now = std::chrono::system_clock::now();
        auto in_time_t = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << std::put_time(std::gmtime(&in_time_t), "%Y-%m-%dT%H:%M:%SZ");
        record.finished_at_iso = ss.str();

        record.tachyon_version = "0.8.0-dev";
        
        TelemetryWriter::write_sqlite(record, result);
    }

    // Telemetry finalization
    TelemetryEvent se;
    se.event = "render_session_end";
    se.w = workspace.effective_plan.render_plan.output.profile.width.value_or(0);
    se.h = workspace.effective_plan.render_plan.output.profile.height.value_or(0);
    se.total_ms = result.wall_time_total_ms;
    se.setup_ms = result.io_read_ms;
    se.composite_ms = result.frame_execution_ms;
    se.encode_ms = result.encode_ms;
    RenderTelemetry::get().log(se);

    return result;
}

RenderSessionResult RenderSession::render(
    const SceneSpec& scene,
    const CompiledScene& compiled_scene,
    const RenderExecutionPlan& execution_plan,
    const std::filesystem::path& output_path) {
    
    runtime::RenderWorkerPolicy policy;
    return render(scene, compiled_scene, execution_plan, output_path, 
                  policy.resolve(std::thread::hardware_concurrency()));
}

} // namespace tachyon
