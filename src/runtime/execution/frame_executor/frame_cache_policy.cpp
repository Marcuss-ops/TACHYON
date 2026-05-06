#include "frame_executor_internal.h"

namespace tachyon {

FrameCacheState build_frame_cache_state(
    const CompiledScene& compiled_scene,
    const RenderPlan& plan,
    const FrameRenderTask& task,
    bool diagnostics_enabled) {
    FrameCacheState state;
    state.diagnostics_enabled = diagnostics_enabled;
    state.composition_builder.enable_manifest(diagnostics_enabled);
    state.composition_builder.add_u64(compiled_scene.scene_hash);
    state.composition_builder.add_u32(compiled_scene.header.layout_version);
    state.composition_builder.add_u32(compiled_scene.header.compiler_version);
    state.composition_builder.add_string(plan.quality_tier);
    state.composition_builder.add_string(plan.compositing_alpha_mode);
    state.composition_key = state.composition_builder.finish();

    state.frame_builder = state.composition_builder;
    state.frame_builder.add_u64(static_cast<std::uint64_t>(task.frame_number));
    state.frame_key = state.frame_builder.finish();
    return state;
}

} // namespace tachyon
