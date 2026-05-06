#include "tachyon/runtime/runtime_facade.h"
#include "tachyon/runtime/compiler/scene_compiler.h"
#include "tachyon/runtime/execution/native_render.h"
#include "tachyon/runtime/execution/frames/frame_executor.h"
#include "tachyon/core/spec/validation/scene_validator.h"
#include "tachyon/core/cli_scene_loader.h"
#include "tachyon/background_catalog.h"
#include "tachyon/transition_catalog.h"
#include "tachyon/runtime/frame_arena.h"
#include "tachyon/runtime/cache/frame_cache.h"
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
    ResolutionResult<ExecutedFrame> result;
    
    // Setup minimal execution context for a single frame
    FrameArena arena;
    FrameCache cache; // Transient cache for one-off render; production should use persistent
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
    plan.composition_target = comp.node.node_id;
    // plan.scene_spec = ... (SceneCompiler should ideally embed or we pass it)
    
    FrameRenderTask task;
    task.frame_number = frame;
    task.time_seconds = static_cast<double>(frame) / static_cast<double>(comp.fps);
    
    FrameExecutor executor(arena, cache);
    result.value = executor.execute(scene, plan, task, context);
    
    if (!result.value->success) {
        result.diagnostics.add_error("RENDER", result.value->error);
    }
    
    return result;
}

ResolutionResult<std::monostate> RuntimeFacade::export_video(const CompiledScene& /*scene*/, const ExportOptions& options) {
    ResolutionResult<std::monostate> result;
    
    // Legacy bridge to NativeRenderer which currently expects SceneSpec.
    // TODO: refactor NativeRenderer to accept CompiledScene.
    // For now, we return "Not implemented for CompiledScene yet" or similar.
    result.diagnostics.add_error("FACADE", "export_video for CompiledScene not implemented. Use SceneSpec pipeline.");
    
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

    const RenderSessionResult session_result = NativeRenderer::render(scene, job, native_options);

    result.success = session_result.output_error.empty();
    result.error_message = session_result.output_error;
    
    return result;
}

} // namespace tachyon
