#pragma once

#include "tachyon/core/api.h"
#include "tachyon/core/spec/schema/objects/scene_spec.h"
#include "tachyon/runtime/core/data/compiled_scene.h"
#include "tachyon/runtime/execution/frames/frame_executor.h"
#include "tachyon/runtime/core/diagnostics/diagnostics.h"
#include <string>
#include <vector>
#include <memory>

#include <variant>

namespace tachyon {

/**
 * @brief Options for video export through the facade.
 */
struct ExportOptions {
    std::string composition_id;
    int start_frame{0};
    int end_frame{0};
    std::string output_path;
    int width{0};  // 0 = use composition default
    int height{0}; // 0 = use composition default
    int worker_count{0}; // 0 = auto
};

/**
 * @brief Unified runtime facade for Tachyon engine.
 * 
 * This is the singular, hardened entry point for all engine consumers.
 * It enforces the canonical pipeline: Spec -> CompiledScene -> Frame / Video.
 */
class TACHYON_API RuntimeFacade {
public:
    static RuntimeFacade& instance();

    /**
     * @brief Compiles a scene specification into its runtime representation.
     */
    [[nodiscard]] ResolutionResult<CompiledScene> compile_scene(const SceneSpec& spec);

    /**
     * @brief Renders a single frame from a compiled scene.
     */
    [[nodiscard]] ResolutionResult<ExecutedFrame> render_frame(const CompiledScene& scene, int frame);

    /**
     * @brief Exports a video range to disk.
     */
    [[nodiscard]] ResolutionResult<std::monostate> export_video(const CompiledScene& scene, const ExportOptions& options);

    /**
     * @brief Result of a facade-level validation.
     */
    struct FacadeValidationResult {
        bool valid{false};
        std::vector<std::string> errors;
        std::vector<std::string> warnings;
    };

    /**
     * @brief Validates a scene specification.
     */
    [[nodiscard]] FacadeValidationResult validate_scene(const SceneSpec& spec);

    // Legacy / CLI support structures (refactored to use new pipeline internally)
    struct RenderRequest {
        std::string scene_path;
        std::string output_path;
        int width{1920};
        int height{1080};
        int start_frame{0};
        int end_frame{100};
        std::string preset;
    };

    struct RenderResult {
        bool success{false};
        std::string error_message;
    };

    RenderResult render_legacy(const RenderRequest& request);
};

} // namespace tachyon
