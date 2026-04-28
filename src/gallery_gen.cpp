#include "tachyon/background_generator.h"
#include "tachyon/core/spec/schema/objects/scene_spec.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <filesystem>

// Forward declarations of serialization functions (defined in TachyonCore)
namespace tachyon {
    void to_json(nlohmann::json& j, const SceneSpec& s);
}

void save_scene(const tachyon::SceneSpec& scene, const std::string& path) {
    nlohmann::json j = scene;
    std::ofstream file(path);
    file << j.dump(2);
    std::cout << "Saved: " << path << std::endl;
}

int main() {
    std::filesystem::create_directories("gallery");

    // 1. Aura Modern
    save_scene(tachyon::BackgroundGenerator::GenerateAuraBackground(), "gallery/aura.json");

    // 2. Liquid Deep
    save_scene(tachyon::BackgroundGenerator::GenerateLiquidBackground(), "gallery/liquid.json");

    // 3. Grid (Hexagons)
    save_scene(tachyon::BackgroundGenerator::GenerateGridBackground(1920, 1080, 5.0, 80.0, 2.0, "hexagon"), "gallery/grid_hex.json");

    // 4. Stars
    save_scene(tachyon::BackgroundGenerator::GenerateStarsBackground(1920, 1080, 5.0, 2.0, 0.2), "gallery/stars.json");

    // 5. Stripes
    save_scene(tachyon::BackgroundGenerator::GenerateStripesBackground(1920, 1080, 5.0, 45.0, 1.0), "gallery/stripes.json");

    return 0;
}
