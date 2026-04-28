#pragma once

#include "tachyon/core/spec/schema/objects/scene_spec.h"
#include "tachyon/core/spec/schema/objects/layer_spec.h"
#include "tachyon/renderer2d/spec/gradient_spec.h"
#include <string>
#include <optional>

namespace tachyon {

/**
 * @brief Simple background generator for quick procedural backgrounds.
 */
class BackgroundGenerator {
public:
    static SceneSpec GenerateNoiseBackground(
        int width = 1920, int height = 1080, double duration = 5.0,
        double frequency = 1.0, double amplitude = 50.0, uint64_t seed = 0);
    
    static SceneSpec GenerateGradientBackground(
        int width = 1920, int height = 1080, double duration = 5.0,
        const ColorSpec& color_a = ColorSpec{20, 30, 80, 255},
        const ColorSpec& color_b = ColorSpec{60, 100, 180, 255});
    
    static SceneSpec GenerateWavesBackground(
        int width = 1920, int height = 1080, double duration = 5.0,
        double frequency = 2.0, double amplitude = 30.0);

    // Modern Modular Backgrounds
    static SceneSpec GenerateAuraBackground(
        int width = 1920, int height = 1080, double duration = 5.0,
        const ColorSpec& color_primary = ColorSpec{255, 51, 102, 255},
        const ColorSpec& color_secondary = ColorSpec{51, 102, 255, 255},
        double speed = 0.5, double intensity = 1.0);

    static SceneSpec GenerateLiquidBackground(
        int width = 1920, int height = 1080, double duration = 5.0,
        const ColorSpec& accent_color = ColorSpec{26, 26, 46, 255});

    static SceneSpec GenerateGridBackground(
        int width = 1920, int height = 1080, double duration = 5.0,
        double spacing = 50.0, double border = 1.0, const std::string& shape = "square");

    static SceneSpec GenerateStripesBackground(
        int width = 1920, int height = 1080, double duration = 5.0,
        double angle = 0.0, double speed = 0.5);

    static SceneSpec GenerateStarsBackground(
        int width = 1920, int height = 1080, double duration = 5.0,
        double density = 1.0, double speed = 0.1);

    /**
     * @brief Generic wrapper to create a scene from any library component.
     */
    static SceneSpec GenerateFromComponent(
        const std::string& component_id,
        const std::map<std::string, std::string>& params,
        int width = 1920, int height = 1080, double duration = 5.0);
};

SceneSpec GenerateBackground(int width = 1920, int height = 1080, double duration = 5.0);

} // namespace tachyon
