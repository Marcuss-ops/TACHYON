#include "tachyon/runtime/execution/compiled_frame_program.h"
#include "tachyon/runtime/execution/session/render_session.h"
#include <fstream>

namespace tachyon {

RenderSessionResult CompiledFrameProgram::render_frame(double /*time_sec*/, const std::string& output_path) const {
    RenderSession session;
    return session.render(scene, compiled_scene, exec_plan, output_path);
}

ResolutionResult<CompiledFrameProgram> build_compiled_frame_program(
    const SceneSpec& scene,
    const RenderExecutionPlan& plan
) {
    CompiledFrameProgram program;
    program.scene = scene;
    program.exec_plan = plan;
    
    // Compile scene once
    // Note: actual compilation logic depends on existing compile_scene implementation
    // program.compiled_scene = compile_scene(scene);
    
    ResolutionResult<CompiledFrameProgram> result;
    result.value = std::move(program);
    result.diagnostics = DiagnosticBag{};
    return result;
}

} // namespace tachyon
