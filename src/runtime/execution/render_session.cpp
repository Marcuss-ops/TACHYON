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
#include "tachyon/runtime/profiling/render_profiler.h"
#include "tachyon/renderer2d/effects/core/transitions/basic_transitions.h"
#include "tachyon/renderer2d/effects/core/transitions/artistic_transitions.h"
#include "tachyon/renderer2d/effects/core/transitions/light_leak_transitions.h"
#include "tachyon/runtime/policy/worker_policy.h"
#include "tachyon/runtime/policy/surface_pool_policy.h"
#include "tachyon/runtime/policy/telemetry_policy.h"

// Modular Session Pipeline Steps
#include "tachyon/runtime/execution/session/render_session_state.h"
#include "tachyon/runtime/execution/session/scoped_process_sampler.h"
#include "tachyon/runtime/execution/session/render_session_preflight.h"
#include "tachyon/runtime/execution/session/sink_lifecycle.h"
#include "tachyon/runtime/execution/session/frame_execution_loop.h"
#include "tachyon/runtime/execution/session/audio_export_step.h"
#include "tachyon/runtime/execution/session/render_metrics_collector.h"
#include "tachyon/runtime/execution/session/render_cleanup.h"

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

void configure_render_context(
    RenderSessionState& state,
    profiling::RenderProfiler* profiler,
    std::shared_ptr<SurfacePool> surface_pool,
    const renderer2d::EffectRegistry& effect_registry,
    const TransitionRegistry& transition_registry,
    const presets::TextRegistry* text_registry,
    audio::IAudioExporter* audio_exporter) {
    state.context.policy = state.effective_plan.render_plan.quality_policy;
#ifdef TACHYON_ENABLE_TEXT
    state.context.font_registry = ::tachyon::renderer2d::get_default_font_registry();
#else
    state.context.font_registry = nullptr;
#endif

    state.context.surface_pool = surface_pool;
    state.context.profiler = profiler;
    state.context.effects = renderer2d::create_effect_host(effect_registry);
    state.context.transition_registry = &transition_registry;
    state.context.text_registry = text_registry;
#ifdef TACHYON_ENABLE_MEDIA
    state.context.audio_exporter = audio_exporter;
#endif
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

    const auto session_start = std::chrono::steady_clock::now();
    
    runtime::TelemetryPolicy telemetry_policy;
    ScopedProcessSampler sampler(telemetry_policy.should_sample_process());

    auto result = RenderSessionResult{};

    RenderSessionState state;
    state.effective_plan = execution_plan;
    state.resolved_output_path = RenderSessionPlanResolver::resolve_output_path(output_path, execution_plan.render_plan);
    state.context = RenderContext(m_precomp_cache);
    state.context.text_surface_cache = m_text_surface_cache;
#ifdef TACHYON_ENABLE_MEDIA
    state.prefetcher = m_prefetcher.get();
#endif
    state.session_start = session_start;

    RenderSessionPlanResolver::apply_output_override(state, output_path);

    std::uint32_t w = static_cast<std::uint32_t>(state.effective_plan.render_plan.composition.width);
    std::uint32_t h = static_cast<std::uint32_t>(state.effective_plan.render_plan.composition.height);
    
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

    configure_render_context(state, m_profiler, m_surface_pool, *effect_registry, *transition_registry, text_registry, m_audio_exporter);

    // Phase 0: Preflight (Asset/Font/Writability checks)
    RenderSessionPreflightInput preflight_input{
        scene,
        compiled_scene,
        state.effective_plan,
        state.context
    };

    if (!RenderSessionPreflight::run(preflight_input, result)) {
        return result;
    }

    SinkLifecycle::create(state);

    if (!SinkLifecycle::begin(state, result, m_profiler)) {
        RenderMetricsCollector::finalize_session(state, result, sampler);
        return result;
    }

    FrameExecutionLoop::run(state, compiled_scene, m_cache, budget, m_prefetcher.get(), cancel_flag, result);

    AudioExportStep::run(state, cancel_flag);

    SinkLifecycle::finish(state, result);

    RenderMetricsCollector::finalize_session(state, result, sampler);
    RenderMetricsCollector::log_frame_events(state, result);
    RenderMetricsCollector::persist_telemetry(state, result);

    RenderCleanup::run(state);

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
