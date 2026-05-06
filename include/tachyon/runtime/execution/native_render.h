#pragma once

#include "tachyon/runtime/core/data/compiled_scene.h"
#include "tachyon/core/spec/schema/objects/scene_spec.h"
#include "tachyon/runtime/execution/jobs/render_job.h"
#include "tachyon/runtime/execution/session/render_session.h"
#include "tachyon/runtime/execution/render_progress_sink.h"
#include "tachyon/runtime/core/diagnostics/diagnostics.h"

#include <filesystem>
#include <string>

namespace tachyon {

namespace profiling { class RenderProfiler; }

/**
 * @brief High-level utility for rendering a SceneSpec directly.
 * 
 * This bypasses the legacy JSON-based CLI path and works directly with the 
 * C++ specification structures.
 */
struct NativeRenderOptions {
    std::size_t worker_count{0};
    std::optional<std::size_t> memory_budget_bytes;
    bool verbose{false};
    RenderProgressSink* progress_sink{nullptr};
    profiling::RenderProfiler* profiler{nullptr};
};

class NativeRenderer {
public:
    /**
     * @brief Renders a composition from the given scene.
     * 
     * @param scene The scene specification to render.
     * @param job The render job defining output settings and frame range.
     * @param options Execution options (workers, memory, etc).
     * @return RenderSessionResult containing status and frame info.
     */
    static RenderSessionResult render(
        const SceneSpec& scene,
        const RenderJob& job,
        const NativeRenderOptions& options = NativeRenderOptions());

    static RenderSessionResult render(
        const CompiledScene& scene,
        const RenderJob& job,
        const NativeRenderOptions& options = NativeRenderOptions());

    /**
     * @brief Shortcut to render a single frame to a PNG.
     */
    static bool render_still(
        const SceneSpec& scene,
        const std::string& composition_id,
        std::int64_t frame_number,
        const std::filesystem::path& output_path);
};

} // namespace tachyon
