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

template <typename T>
void save_scene(const T& scene, const std::string& path) {
    nlohmann::json j = scene;
    std::ofstream file(path);
    file << j.dump(2);
    std::cout << "Saved: " << path << std::endl;
}

int main() {
    std::filesystem::create_directories("gallery");

    // Generate ShapeGrid background examples
    tachyon::ShapeGridParams params;
    params.shape = "hexagon";
    params.spacing = 80.0f;
    params.border_width = 2.0f;
    params.speed = 1.0f;
    params.direction = "right";
    params.seed = 42;

    save_scene(tachyon::GenerateShapeGridBackground(params), "gallery/grid_hex.json");

    params.shape = "square";
    params.spacing = 40.0f;
    save_scene(tachyon::GenerateShapeGridBackground(params), "gallery/grid_square.json");

    return 0;
}
