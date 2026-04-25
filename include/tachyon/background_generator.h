#pragma once

#include "tachyon/core/spec/schema/objects/scene_spec.h"
#include "tachyon/core/spec/schema/objects/layer_spec.h"
#include "tachyon/renderer2d/spec/gradient_spec.h"
#include <string>
#include <optional>

namespace tachyon {

/**
 * @brief Simple background generator for quick procedural backgrounds.
 * 
 * Usage:
 *   auto scene = GenerateNoiseBackground(1920, 1080, 5.0);
 *   // Render scene to MP4...
 */
class BackgroundGenerator {
public:
    /**
     * @brief Generate a noise-based procedural background.
     * 
     * @param width Width in pixels
     * @param height Height in pixels
     * @param duration Duration in seconds
     * @param frequency Noise frequency (default 1.0)
     * @param amplitude Noise amplitude (default 50.0)
     * @param seed Random seed (default 0)
     * @return SceneSpec with a single procedural layer
     */
    static SceneSpec GenerateNoiseBackground(
        int width = 1920, int height = 1080, double duration = 5.0,
        double frequency = 1.0, double amplitude = 50.0, uint64_t seed = 0) {
        
        SceneSpec scene;
        scene.project.id = "bg_project";
        scene.project.name = "Generated Background";
        
        CompositionSpec comp;
        comp.id = "main_comp";
        comp.name = "Background";
        comp.width = width;
        comp.height = height;
        comp.frame_rate = FrameRate{30, 1};
        comp.duration = duration;
        
        LayerSpec layer;
        layer.id = "bg_layer";
        layer.name = "Noise Background";
        layer.type = "procedural";
        layer.width = width;
        layer.height = height;
        layer.enabled = true;
        layer.visible = true;
        
        ProceduralSpec proc;
        proc.kind = "noise";
        proc.frequency.value = frequency;
        proc.amplitude.value = amplitude;
        proc.seed = seed;
        proc.color_a = ColorSpec{30, 30, 40, 255};
        proc.color_b = ColorSpec{60, 60, 80, 255};
        layer.procedural = proc;
        
        comp.layers.push_back(layer);
        scene.compositions.push_back(comp);
        
        return scene;
    }
    
    /**
     * @brief Generate a gradient background (animated or static).
     * 
     * @param width Width in pixels
     * @param height Height in pixels
     * @param duration Duration in seconds
     * @param color_a Start color (default dark blue)
     * @param color_b End color (default light blue)
     * @return SceneSpec with a gradient-filled layer
     */
    static SceneSpec GenerateGradientBackground(
        int width = 1920, int height = 1080, double duration = 5.0,
        const ColorSpec& color_a = ColorSpec{20, 30, 80, 255},
        const ColorSpec& color_b = ColorSpec{60, 100, 180, 255}) {
        
        SceneSpec scene;
        scene.project.id = "bg_project";
        scene.project.name = "Generated Background";
        
        CompositionSpec comp;
        comp.id = "main_comp";
        comp.name = "Background";
        comp.width = width;
        comp.height = height;
        comp.frame_rate = FrameRate{30, 1};
        comp.duration = duration;
        
        LayerSpec layer;
        layer.id = "bg_layer";
        layer.name = "Gradient Background";
        layer.type = "solid";
        layer.width = width;
        layer.height = height;
        layer.enabled = true;
        layer.visible = true;
        
        AnimatedGradientSpec grad;
        grad.type = "linear";
        
        AnimatedGradientStop stop1;
        stop1.offset.value = 0.0;
        stop1.color.value = color_a;
        grad.stops.push_back(stop1);
        
        AnimatedGradientStop stop2;
        stop2.offset.value = 1.0;
        stop2.color.value = color_b;
        grad.stops.push_back(stop2);
        
        layer.gradient_fill = grad;
        layer.fill_color = color_a; // fallback
        
        comp.layers.push_back(layer);
        scene.compositions.push_back(comp);
        
        return scene;
    }
    
    /**
     * @brief Generate a waves procedural background.
     */
    static SceneSpec GenerateWavesBackground(
        int width = 1920, int height = 1080, double duration = 5.0,
        double frequency = 2.0, double amplitude = 30.0) {
        
        SceneSpec scene;
        scene.project.id = "bg_project";
        scene.project.name = "Generated Background";
        
        CompositionSpec comp;
        comp.id = "main_comp";
        comp.name = "Background";
        comp.width = width;
        comp.height = height;
        comp.frame_rate = FrameRate{30, 1};
        comp.duration = duration;
        
        LayerSpec layer;
        layer.id = "bg_layer";
        layer.name = "Waves Background";
        layer.type = "procedural";
        layer.width = width;
        layer.height = height;
        layer.enabled = true;
        layer.visible = true;
        
        ProceduralSpec proc;
        proc.kind = "waves";
        proc.frequency.value = frequency;
        proc.amplitude.value = amplitude;
        proc.color_a = ColorSpec{40, 60, 80, 255};
        proc.color_b = ColorSpec{80, 120, 160, 255};
        layer.procedural = proc;
        
        comp.layers.push_back(layer);
        scene.compositions.push_back(comp);
        
        return scene;
    }
};

/**
 * @brief Helper function to generate a simple background (noise type).
 */
inline SceneSpec GenerateBackground(
    int width = 1920, int height = 1080, double duration = 5.0) {
    return BackgroundGenerator::GenerateNoiseBackground(width, height, duration);
}

} // namespace tachyon
