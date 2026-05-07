#include "tachyon/runtime/runtime_facade.h"
#include "tachyon/runtime/compiler/scene_compiler.h"
#include "tachyon/runtime/execution/native_render.h"
#include "tachyon/runtime/execution/frames/frame_executor.h"
#include "tachyon/runtime/execution/jobs/render_job.h"
#include "tachyon/core/spec/validation/scene_validator.h"
#include "tachyon/core/cli_scene_loader.h"
#include "tachyon/transition_registry.h"
#include "tachyon/core/transition/transition_descriptor.h"
#include "tachyon/runtime/frame_arena.h"
#include "tachyon/runtime/cache/frame_cache.h"
#include "tachyon/runtime/execution/session/render_session.h"
#include <iostream>
#include <sstream>

namespace tachyon {

RuntimeFacade& RuntimeFacade::instance() {
    static RuntimeFacade instance;
    return instance;
}

ResolutionResult<CompiledScene> RuntimeFacade::compile_scene(const SceneSpec& spec) {
    SceneCompiler compiler;
    return compiler.compile(spec);
}

ResolutionResult<ExecutedFrame> RuntimeFacade::render_frame(const CompiledScene& scene, int frame) {
    static thread_local RenderSession session;
    return render_frame(scene, frame, session);
}

ResolutionResult<ExecutedFrame> RuntimeFacade::render_frame(const CompiledScene& scene, int frame, RenderSession& session) {
    ResolutionResult<ExecutedFrame> result;
    
    // Setup minimal execution context for a single frame
    FrameArena arena;
    RenderContext context;
    
    // We need a RenderPlan and FrameRenderTask
    // These are usually built by the higher-level pipeline.
    // For the facade, we'll build a default plan for the first composition.
    if (scene.compositions.empty()) {
        result.diagnostics.add_error("FACADE", "Scene has no compositions.");
        return result;
    }
    
    const auto& comp = scene.compositions.front();
    RenderPlan plan;
    plan.composition_target = std::to_string(comp.node.node_id);
    plan.composition.id = plan.composition_target;
    plan.composition.name = plan.composition_target;
    plan.composition.width = comp.width;
    plan.composition.height = comp.height;
    plan.composition.duration = comp.duration;
    plan.composition.frame_rate.numerator = comp.fps > 0 ? comp.fps : 60U;
    plan.composition.frame_rate.denominator = 1;

    FrameRenderTask task;
    task.frame_number = frame;
    task.time_seconds = static_cast<double>(frame) / static_cast<double>(comp.fps > 0 ? comp.fps : 60U);
    
    FrameExecutor executor(arena, session.cache());
    result.value = executor.execute(scene, plan, task, context);
    
    if (!result.value->success) {
        result.diagnostics.add_error("RENDER", result.value->error);
    }
    
    return result;
}

ResolutionResult<std::monostate> RuntimeFacade::export_video(const CompiledScene& scene, const ExportOptions& options) {
    ResolutionResult<std::monostate> result;

    if (scene.compositions.empty()) {
        result.diagnostics.add_error("FACADE", "Compiled scene has no compositions.");
        return result;
    }

    RenderJob job = RenderJobBuilder::video_export(options.composition_id, {options.start_frame, options.end_frame}, options.output_path);
    if (options.width > 0) {
        job.output.profile.width = options.width;
    }
    if (options.height > 0) {
        job.output.profile.height = options.height;
    }

    NativeRenderOptions native_options;
    native_options.worker_count = options.worker_count > 0 ? static_cast<std::size_t>(options.worker_count) : 0U;

    TransitionRegistry transition_registry;
    register_builtin_transitions(transition_registry);

    const auto render_result = NativeRenderer::render(scene, job, transition_registry, native_options);
    result.diagnostics.append(render_result.diagnostics);
    if (!render_result.output_error.empty()) {
        result.diagnostics.add_error("FACADE", render_result.output_error);
    } else {
        result.value = std::monostate{};
    }

    return result;
}

RuntimeFacade::FacadeValidationResult RuntimeFacade::validate_scene(const SceneSpec& spec) {
    core::SceneValidator validator;
    auto res = validator.validate(spec);
    
    FacadeValidationResult facade_res;
    facade_res.valid = res.is_valid();
    for (const auto& issue : res.issues) {
        if (issue.severity >= core::ValidationIssue::Severity::Error) {
            facade_res.errors.push_back(issue.message);
        } else {
            facade_res.warnings.push_back(issue.message);
        }
    }
    return facade_res;
}

RuntimeFacade::RenderResult RuntimeFacade::render_legacy(const RenderRequest& request) {
    RenderResult result;
    SceneLoadOptions load_opts;
    load_opts.cpp_path = request.scene_path;
    load_opts.preset_id = request.preset.empty() ? std::nullopt : std::optional<std::string>(request.preset);

    std::stringstream out_ss, err_ss;
    auto loaded = load_scene_for_cli(load_opts, SceneLoadMode::Render, out_ss, err_ss);
    
    if (!loaded.success) {
        result.success = false;
        result.error_message = "Scene load failed: " + err_ss.str();
        return result;
    }

    const auto& scene = loaded.context->scene;
    if (scene.compositions.empty()) {
        result.success = false;
        result.error_message = "Scene has no compositions.";
        return result;
    }

    const auto& comp = scene.compositions.front();
    FrameRange range = {request.start_frame, request.end_frame};
    
    RenderJob job = RenderJobBuilder::video_export(comp.id, range, request.output_path);

    NativeRenderOptions native_options;
    native_options.worker_count = 0; 
    native_options.verbose = false;

    TransitionRegistry transition_registry;
    register_builtin_transitions(transition_registry);

    const RenderSessionResult session_result = NativeRenderer::render(scene, job, transition_registry, native_options);

    result.success = session_result.output_error.empty();
    result.error_message = session_result.output_error;
    
    return result;
}

} // namespace tachyon
