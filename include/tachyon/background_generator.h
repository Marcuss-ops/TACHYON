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
};

SceneSpec GenerateBackground(int width = 1920, int height = 1080, double duration = 5.0);

} // namespace tachyon
