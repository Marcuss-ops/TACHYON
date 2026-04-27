#include "tachyon/background_generator.h"

namespace tachyon {

SceneSpec BackgroundGenerator::GenerateNoiseBackground(
    int width, int height, double duration,
    double frequency, double amplitude, uint64_t seed) {
    
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
    proc.color_a.value = ColorSpec{30, 30, 40, 255};
    proc.color_b.value = ColorSpec{60, 60, 80, 255};
    layer.procedural = proc;
    
    comp.layers.push_back(layer);
    scene.compositions.push_back(comp);
    
    return scene;
}

SceneSpec BackgroundGenerator::GenerateGradientBackground(
    int width, int height, double duration,
    const ColorSpec& color_a, const ColorSpec& color_b) {
    
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
    layer.fill_color.value = color_a; // fallback
    
    comp.layers.push_back(layer);
    scene.compositions.push_back(comp);
    
    return scene;
}

SceneSpec BackgroundGenerator::GenerateWavesBackground(
    int width, int height, double duration,
    double frequency, double amplitude) {
    
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
    proc.color_a.value = ColorSpec{40, 60, 80, 255};
    proc.color_b.value = ColorSpec{80, 120, 160, 255};
    layer.procedural = proc;
    
    comp.layers.push_back(layer);
    scene.compositions.push_back(comp);
    
    return scene;
}

SceneSpec GenerateBackground(int width, int height, double duration) {
    return BackgroundGenerator::GenerateNoiseBackground(width, height, duration);
}

} // namespace tachyon
