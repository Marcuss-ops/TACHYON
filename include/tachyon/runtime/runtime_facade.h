#pragma once

#include <string>
#include <vector>
#include <optional>

namespace tachyon {

// Forward declarations
struct RenderRequest;
struct RenderResult;
struct PreviewRequest;
struct PreviewResult;
struct ValidationRequest;
struct ValidationResult;
struct PresetListResult;

/**
 * @brief Unified runtime facade for Tachyon engine.
 * 
 * This is the single entry point for:
 * - CLI tools
 * - Future HTTP API
 * - End-to-end tests
 * - Preview tools
 * 
 * All external consumers should use this facade instead of
 * calling internal functions directly.
 */
class RuntimeFacade {
public:
    virtual ~RuntimeFacade() = default;

    /**
     * @brief Render a scene or composition.
     * @param request Render parameters
     * @return Render result with output path or error
     */
    virtual RenderResult render(const RenderRequest& request) = 0;

    /**
     * @brief Generate a quick preview (lower quality, faster).
     * @param request Preview parameters
     * @return Preview result
     */
    virtual PreviewResult preview(const PreviewRequest& request) = 0;

    /**
     * @brief Validate a scene or preset without rendering.
     * @param request Validation parameters
     * @return Validation result
     */
    virtual ValidationResult validate(const ValidationRequest& request) = 0;

    /**
     * @brief List available presets.
     * @return List of presets
     */
    virtual PresetListResult list_presets() = 0;
};

// Request/Response types

struct RenderRequest {
    std::string scene_path;
    std::string output_path;
    int width{1920};
    int height{1080};
    float fps{30.0f};
    int start_frame{0};
    int end_frame{100};
    bool enable_3d{false};
    std::string preset;
};

struct RenderResult {
    bool success{false};
    std::string output_path;
    std::string error_message;
    double render_time_ms{0.0};
};

struct PreviewRequest {
    std::string preset_id;
    std::string preset_type; // "transition", "background", "text"
    int width{960};   // Default 540p width
    int height{540};  // 540p preview
    int duration_frames{30};
};

struct PreviewResult {
    bool success{false};
    std::vector<std::string> frame_paths; // Paths to preview frames
    std::string error_message;
};

struct ValidationRequest {
    std::string scene_path;
    bool strict_mode{false};
};

struct ValidationResult {
    bool valid{false};
    std::vector<std::string> errors;
    std::vector<std::string> warnings;
};

struct PresetListResult {
    struct PresetInfo {
        std::string id;
        std::string name;
        std::string description;
        std::string category;
    };
    std::vector<PresetInfo> presets;
};

} // namespace tachyon
