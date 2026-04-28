#include "tachyon/background_generator.h"
#include <sstream>
#include <iomanip>

namespace tachyon {

static std::string ColorToHex(const ColorSpec& c) {
    std::ostringstream oss;
    oss << "#" 
        << std::setfill('0') << std::setw(2) << std::hex << (int)c.r
        << std::setfill('0') << std::setw(2) << std::hex << (int)c.g
        << std::setfill('0') << std::setw(2) << std::hex << (int)c.b;
    return oss.str();
}

static SceneSpec CreateBaseScene(int width, int height, double duration) {
    SceneSpec scene;
    scene.project.id = "bg_project";
    scene.project.name = "Generated Background";
    
    CompositionSpec comp;
    comp.id = "main_comp";
    comp.name = "Background";
    comp.width = width;
    comp.height = height;
    comp.frame_rate = FrameRate{60, 1}; // Standard 60fps for smooth procedural movement
    comp.duration = duration;
    
    scene.compositions.push_back(comp);
    return scene;
}

SceneSpec BackgroundGenerator::GenerateNoiseBackground(
    int width, int height, double duration,
    double frequency, double amplitude, uint64_t seed) {
    
    return GenerateFromComponent("bg_noise", {
        {"frequency", std::to_string(frequency)},
        {"amplitude", std::to_string(amplitude)},
        {"seed", std::to_string(seed)}
    }, width, height, duration);
}

SceneSpec BackgroundGenerator::GenerateGradientBackground(
    int width, int height, double duration,
    const ColorSpec& color_a, const ColorSpec& color_b) {
    
    return GenerateFromComponent("bg_gradient", {
        {"color_a", ColorToHex(color_a)},
        {"color_b", ColorToHex(color_b)}
    }, width, height, duration);
}

SceneSpec BackgroundGenerator::GenerateWavesBackground(
    int width, int height, double duration,
    double frequency, double amplitude) {
    
    return GenerateFromComponent("bg_waves", {
        {"frequency", std::to_string(frequency)},
        {"amplitude", std::to_string(amplitude)}
    }, width, height, duration);
}

SceneSpec BackgroundGenerator::GenerateAuraBackground(
    int width, int height, double duration,
    const ColorSpec& color_primary, const ColorSpec& color_secondary,
    double speed, double intensity) {
    
    return GenerateFromComponent("bg_aura_modern", {
        {"color_primary", ColorToHex(color_primary)},
        {"color_secondary", ColorToHex(color_secondary)},
        {"speed", std::to_string(speed)},
        {"intensity", std::to_string(intensity)}
    }, width, height, duration);
}

SceneSpec BackgroundGenerator::GenerateLiquidBackground(
    int width, int height, double duration,
    const ColorSpec& accent_color) {
    
    return GenerateFromComponent("bg_liquid_deep", {
        {"accent_color", ColorToHex(accent_color)}
    }, width, height, duration);
}

SceneSpec BackgroundGenerator::GenerateGridBackground(
    int width, int height, double duration,
    double spacing, double border, const std::string& shape) {
    
    return GenerateFromComponent("bg_grid", {
        {"spacing", std::to_string(spacing)},
        {"border", std::to_string(border)},
        {"shape", shape}
    }, width, height, duration);
}

SceneSpec BackgroundGenerator::GenerateStripesBackground(
    int width, int height, double duration,
    double angle, double speed) {
    
    return GenerateFromComponent("bg_stripes", {
        {"angle", std::to_string(angle)},
        {"speed", std::to_string(speed)}
    }, width, height, duration);
}

SceneSpec BackgroundGenerator::GenerateStarsBackground(
    int width, int height, double duration,
    double density, double speed) {
    
    return GenerateFromComponent("bg_stars", {
        {"density", std::to_string(density)},
        {"speed", std::to_string(speed)}
    }, width, height, duration);
}

SceneSpec BackgroundGenerator::GenerateFromComponent(
    const std::string& component_id,
    const std::map<std::string, std::string>& params,
    int width, int height, double duration) {
    
    SceneSpec scene = CreateBaseScene(width, height, duration);
    
    ComponentInstanceSpec inst;
    inst.component_id = component_id;
    inst.instance_id = "main_background_" + component_id;
    inst.param_values = params;
    
    scene.compositions[0].component_instances.push_back(inst);
    
    return scene;
}

SceneSpec GenerateBackground(int width, int height, double duration) {
    return BackgroundGenerator::GenerateAuraBackground(width, height, duration);
}

} // namespace tachyon