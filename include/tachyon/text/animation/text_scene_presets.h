#pragma once

#include "tachyon/background_generator.h"
#include "tachyon/core/spec/schema/objects/background_spec.h"
#include "tachyon/core/spec/schema/objects/scene_spec.h"
#include "tachyon/text/animation/text_presets.h"

#include <optional>
#include <string>

namespace tachyon::text {

struct TextScenePresetOptions {
    std::string text{"Il testo leggero entra, respira e resta pulito."};
    std::string project_id{"text_enhance_preset"};
    std::string project_name{"Text Enhance Preset"};
    std::string scene_id{"main"};
    std::string scene_name{"Main"};

    int width{1920};
    int height{1080};
    double duration_seconds{5.0};
    FrameRate frame_rate{30, 1};

    std::uint32_t font_size{88};
    std::string font_id{};
    std::int64_t text_width{1440};
    std::int64_t text_height{220};
    std::int64_t text_position_x{240};
    std::int64_t text_position_y{430};

    std::optional<BackgroundSpec> clear_background{BackgroundSpec::from_string("#0d1117")};
    bool use_procedural_background{true};

    // Procedural background kind: "aura" (default) or "grid" (ShapeGrid)
    std::string procedural_kind{"aura"};

    // Common procedural parameters
    ColorSpec procedural_color_a{14, 20, 32, 255};
    ColorSpec procedural_color_b{20, 33, 53, 255};
    ColorSpec procedural_color_c{8, 10, 18, 255};
    double procedural_speed{0.12};
    double procedural_frequency{0.80};
    double procedural_amplitude{0.82};
    double procedural_scale{1.10};
    double procedural_grain{0.02};

    // ShapeGrid-specific parameters (used when procedural_kind == "grid")
    ShapeGridParams shape_grid_params{};

    std::vector<TextAnimatorSpec> text_animators;
};

[[nodiscard]] LayerSpec make_enhance_text_background_layer(const TextScenePresetOptions& options = {});

[[nodiscard]] LayerSpec make_enhance_text_layer(const TextScenePresetOptions& options = {});

[[nodiscard]] SceneSpec make_enhance_text_scene(const TextScenePresetOptions& options = {});

[[nodiscard]] SceneSpec make_minimal_text_scene(const TextScenePresetOptions& options = {});

} // namespace tachyon::text
