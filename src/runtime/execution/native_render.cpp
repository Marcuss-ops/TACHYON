#include "tachyon/runtime/execution/native_render.h"
#include "tachyon/runtime/compiler/scene_compiler.h"
#include "tachyon/runtime/execution/planning/render_plan.h"
#include "tachyon/runtime/execution/render_progress_sink.h"
#include "tachyon/runtime/execution/jobs/render_job.h"
#include "tachyon/runtime/profiling/render_profiler.h"
#include "tachyon/runtime/policy/worker_policy.h"
#include "tachyon/runtime/registry/engine_registry.h"
#include <chrono>
#include <string>
#include <vector>
#include <thread>
#include <mutex>

namespace tachyon {

namespace {

RenderProgressSink* get_sink(RenderProgressSink* sink) {
    static ConsoleRenderProgressSink console_sink;
    return sink ? sink : &console_sink;
}


const CompiledComposition* find_compiled_composition(const CompiledScene& scene, const std::string& target_id) {
    if (scene.compositions.empty()) {
        return nullptr;
    }

    if (!target_id.empty()) {
        for (const auto& comp : scene.compositions) {
            if (comp.composition_id == target_id) {
                return &comp;
            }
        }
    }

    // Fallback if ID not found but we have compositions
    return &scene.compositions.front();
}

RenderPlan make_compiled_scene_plan(
    const CompiledScene& scene,
    const RenderJob& job,
    ResolutionResult<RenderPlan>& result,
    const SceneSpec* spec) {
    RenderPlan plan;
    const CompiledComposition* composition = find_compiled_composition(scene, job.composition_target);
    if (!composition) {
        result.diagnostics.add_error("plan.composition_missing", "compiled scene has no compositions");
        return plan;
    }

    plan.job_id = job.job_id;
    plan.scene_ref = job.scene_ref;
    plan.composition_target = job.composition_target;
    plan.composition.id = job.composition_target;
    plan.composition.name = job.composition_target;
    plan.composition.width = composition->width;
    plan.composition.height = composition->height;
    plan.composition.duration = composition->duration;
    plan.composition.frame_rate.numerator = composition->fps > 0 ? static_cast<std::int64_t>(composition->fps) : 60;
    plan.composition.frame_rate.denominator = 1;
    plan.frame_range = job.frame_range;
    plan.output = job.output;
    plan.quality_tier = job.quality_tier;
    plan.quality_policy = make_quality_policy(plan.quality_tier);
    plan.compositing_alpha_mode = job.compositing_alpha_mode;
    plan.working_space = job.working_space;
    plan.motion_blur_enabled = job.motion_blur_enabled;
    plan.motion_blur_samples = job.motion_blur_samples > 0 ? job.motion_blur_samples : (job.motion_blur_enabled ? 8 : 0);
    plan.motion_blur_shutter_angle = job.motion_blur_shutter_angle;
    plan.motion_blur_curve = job.motion_blur_curve;
    plan.seed_policy_mode = job.seed_policy_mode;
    plan.compatibility_mode = job.compatibility_mode;
    plan.scene_spec = spec;
    plan.scene_hash = scene.scene_hash;
    plan.contract_version = scene.header.version;
    plan.proxy_enabled = job.proxy_enabled;
    plan.variables = job.variables;
    plan.string_variables = job.string_variables;
    plan.layer_overrides = job.layer_overrides;
    return plan;
}

RenderSessionResult render_with_session(
    const CompiledScene& compiled_scene,
    const RenderJob& job,
    const NativeRenderOptions& options,
    RenderSession& session,
    const SceneSpec* spec = nullptr) {
    RenderSessionResult result;
    RenderProgressSink* sink = get_sink(options.progress_sink);
    const auto total_start = std::chrono::high_resolution_clock::now();
    RenderJob resolved_job = job;
    apply_output_preset(resolved_job.output.profile);

    profiling::ProfileScope total_scope(options.profiler, profiling::ProfileEventType::Phase, "native_render_total");

    sink->on_phase_start(RenderPhase::BuildRenderPlan);
    ResolutionResult<RenderPlan> plan_result;
    {
        profiling::ProfileScope scope(options.profiler, profiling::ProfileEventType::Phase, "build_render_plan");
        plan_result.value = make_compiled_scene_plan(compiled_scene, resolved_job, plan_result, spec);
    }
    if (!plan_result.diagnostics.ok() || !plan_result.value.has_value()) {
        sink->on_message("Render plan build failed!");
        result.diagnostics.append(plan_result.diagnostics);
        return result;
    }
    sink->on_phase_complete(RenderPhase::BuildRenderPlan, 0.0);

    sink->on_phase_start(RenderPhase::BuildExecutionPlan);
    const auto execution_result = build_render_execution_plan(*plan_result.value, compiled_scene.assets.size());
    if (!execution_result.value.has_value()) {
        sink->on_message("Execution plan build failed!");
        result.diagnostics.append(execution_result.diagnostics);
        return result;
    }
    sink->on_phase_complete(RenderPhase::BuildExecutionPlan, 0.0);

    if (options.profiler) {
        session.set_profiler(options.profiler);
    }
    if (options.memory_budget_bytes.has_value()) {
        session.set_memory_budget_bytes(*options.memory_budget_bytes);
    }

    ::tachyon::runtime::RenderWorkerPolicy policy;
    policy.requested_workers = options.worker_count;
    
    const auto budget = policy.resolve(std::thread::hardware_concurrency());

    if (options.verbose) {
        sink->on_message("Starting render: " + resolved_job.job_id);
        sink->on_message("Composition: " + resolved_job.composition_target);
        sink->on_message("Frames: " + std::to_string(resolved_job.frame_range.start) + " -> " + std::to_string(resolved_job.frame_range.end));
        sink->on_message("Output: " + resolved_job.output.destination.path);
        sink->on_message("Budget resolved: frames=" + std::to_string(budget.frame_concurrency) + 
                         ", pixels/frame=" + std::to_string(budget.pixel_concurrency));
    }

    sink->on_phase_start(RenderPhase::Render);
    result = session.render(
        spec ? *spec : SceneSpec{},
        compiled_scene,
        *execution_result.value,
        resolved_job.output.destination.path,
        budget);
    sink->on_phase_complete(RenderPhase::Render);

    const auto total_end = std::chrono::high_resolution_clock::now();
    result.wall_time_total_ms = std::chrono::duration<double, std::milli>(total_end - total_start).count();
    if (result.frames_written > 0) {
        result.wall_time_per_frame_ms = result.wall_time_total_ms / result.frames_written;
    }
    return result;
}

} // anonymous namespace

RenderSessionResult NativeRenderer::render(
    const SceneSpec& scene,
    const RenderJob& job,
    const NativeRenderOptions& options) {
    auto bundle = runtime::create_default_runtime_registry_bundle();
#ifdef TACHYON_ENABLE_TEXT
    return render(scene, job, bundle.transitions, *bundle.text_registry, options);
#else
    RenderSession session;
    session.set_transition_registry(&bundle.transitions);
    SceneCompiler compiler;
    const auto compiled_result = compiler.compile(scene);
    if (!compiled_result.ok()) return {};
    return render_with_session(*compiled_result.value, job, options, session, &scene);
#endif
}

RenderSessionResult NativeRenderer::render(
    const CompiledScene& scene,
    const RenderJob& job,
    const NativeRenderOptions& options) {
    auto bundle = runtime::create_default_runtime_registry_bundle();
#ifdef TACHYON_ENABLE_TEXT
    return render(scene, job, bundle.transitions, *bundle.text_registry, options);
#else
    RenderSession session;
    session.set_transition_registry(&bundle.transitions);
    return render_with_session(scene, job, options, session);
#endif
}

RenderSessionResult NativeRenderer::render(
    const SceneSpec& scene,
    const RenderJob& job,
    TransitionRegistry& transition_registry,
    presets::TextRegistry& text_registry,
    const NativeRenderOptions& options) {
#ifdef TACHYON_ENABLE_TEXT
    RenderJob resolved_job = job;
    apply_output_preset(resolved_job.output.profile);
    RenderSessionResult result;
    const auto preflight_result = validate_render_preflight(scene, resolved_job);
    result.diagnostics.append(preflight_result.diagnostics);
    if (!preflight_result.ok()) {
        get_sink(options.progress_sink)->on_message("Preflight validation failed!");
        return result;
    }

    SceneCompiler compiler;
    const auto compiled_result = compiler.compile(scene);
    if (!compiled_result.ok()) {
        get_sink(options.progress_sink)->on_message("Compilation failed!");
        result.diagnostics.append(compiled_result.diagnostics);
        return result;
    }

    RenderSession session;
    session.set_transition_registry(&transition_registry);
    session.set_text_registry(&text_registry);
    return render_with_session(*compiled_result.value, resolved_job, options, session, &scene);
#else
    (void)scene; (void)job; (void)transition_registry; (void)text_registry; (void)options;
    return {};
#endif
}

RenderSessionResult NativeRenderer::render(
    const CompiledScene& scene,
    const RenderJob& job,
    TransitionRegistry& transition_registry,
    presets::TextRegistry& text_registry,
    const NativeRenderOptions& options) {
#ifdef TACHYON_ENABLE_TEXT
    RenderSession session;
    session.set_transition_registry(&transition_registry);
    session.set_text_registry(&text_registry);
    return render_with_session(scene, job, options, session);
#else
    (void)scene; (void)job; (void)transition_registry; (void)text_registry; (void)options;
    return {};
#endif
}

bool NativeRenderer::render_still(
    const SceneSpec& scene,
    const std::string& composition_id,
    std::int64_t frame_number,
    const std::filesystem::path& output_path,
    TransitionRegistry& transition_registry,
    presets::TextRegistry& text_registry) {
#ifdef TACHYON_ENABLE_TEXT
    RenderJob job = RenderJobBuilder::still_image(composition_id, frame_number, output_path.string());
    const auto result = render(scene, job, transition_registry, text_registry);
    return result.output_error.empty() && (!result.frames.empty() || result.frames_written > 0);
#else
    (void)scene; (void)composition_id; (void)frame_number; (void)output_path; (void)transition_registry; (void)text_registry;
    return false;
#endif
}

bool NativeRenderer::render_still(
    const SceneSpec& scene,
    const std::string& composition_id,
    std::int64_t frame_number,
    const std::filesystem::path& output_path,
    TransitionRegistry& transition_registry) {
    
    RenderJob job = RenderJobBuilder::still_image(composition_id, frame_number, output_path.string());
    
    RenderSession session;
    session.set_transition_registry(&transition_registry);
    
    SceneCompiler compiler;
    const auto compiled_result = compiler.compile(scene);
    if (!compiled_result.ok()) return false;

    const auto result = render_with_session(*compiled_result.value, job, NativeRenderOptions(), session, &scene);
    return result.output_error.empty() && (!result.frames.empty() || result.frames_written > 0);
}

} // namespace tachyon
