#include "tachyon/renderer2d/evaluated_composition/rendering/pipeline/scene3d_bridge.h"
#include "tachyon/renderer3d/core/evaluated_scene_3d.h"
#include "tachyon/core/scene/state/evaluated_state.h"
#include "tachyon/text/fonts/core/font_registry.h"

#include <vector>
#include <string>
#include <filesystem>
#include <iostream>

namespace {

int g_failures = 0;

void check_true(bool condition, const std::string& message) {
    if (!condition) {
        ++g_failures;
        std::cerr << "FAIL: " << message << '\n';
    }
}

std::filesystem::path fixture_path() {
    return std::filesystem::path(TACHYON_TESTS_SOURCE_DIR) / "fixtures" / "fonts" / "minimal_5x7.bdf";
}

} // namespace

bool run_scene3d_bridge_tests() {
    g_failures = 0;

    // CameraContract test
    {
        tachyon::scene::EvaluatedCameraState camera;
        camera.available = true;
        camera.position = {1.0f, 2.0f, 3.0f};
        camera.point_of_interest = {0.0f, 0.0f, 0.0f};
        camera.angle_of_view = 45.0f;
        camera.focal_length = 50.0f;
        camera.focus_distance = 100.0f;
        camera.aperture = 2.8f;
        camera.layer_id = "camera1";

        tachyon::scene::EvaluatedCompositionState state;
        state.camera = camera;

        const auto result = tachyon::build_camera_3d(camera, state);
        check_true(result.position.x == 1.0f, "Camera position.x should be 1.0");
        check_true(result.target.x == 0.0f, "Camera target.x should be 0.0");
        check_true(result.is_active_camera, "Camera should be active");
        check_true(result.camera_id == "camera1", "Camera ID should match");
    }

    // MaterialContract test
    {
        tachyon::scene::EvaluatedCompositionState state;
        tachyon::Scene3DBridgeInput input;
        tachyon::scene::EvaluatedLayerState layer;
        layer.type = tachyon::scene::LayerType::Solid;
        layer.fill_color = {255, 0, 0, 255};
        layer.opacity = 0.5;
        layer.material.metallic = 0.8f;
        layer.material.roughness = 0.2f;
        layer.material.emission = 1.0f;
        layer.material.transmission = 0.5f;
        layer.material.ior = 1.5f;
        layer.world_matrix = tachyon::math::Matrix4x4::identity();
        layer.previous_world_matrix = tachyon::math::Matrix4x4::identity();

        std::vector<std::size_t> block_indices = {0};
        std::vector<bool> visible_3d_layers = {true};
        tachyon::renderer2d::RenderContext2D context;

        input.state = &state;
        input.block_indices = &block_indices;
        input.visible_3d_layers = &visible_3d_layers;
        input.context = &context;
        input.plan = nullptr;
        input.task = nullptr;

        state.layers.push_back(layer);

        const auto instances = tachyon::build_instances_3d(input);
        check_true(instances.size() == 1U, "Should have 1 instance");
        const auto& inst = instances.front();
        check_true(inst.material.base_color.r == 255, "Material red should be 255");
        check_true(inst.material.opacity == 0.5f, "Material opacity should be 0.5");
        check_true(inst.material.metallic == 0.8f, "Material metallic should be 0.8");
        check_true(inst.material.roughness == 0.2f, "Material roughness should be 0.2");
        check_true(inst.material.emission_strength == 1.0f, "Material emission should be 1.0");
        check_true(inst.material.transmission == 0.5f, "Material transmission should be 0.5");
        check_true(inst.material.ior == 1.5f, "Material IOR should be 1.5");
    }

    // TextExtrusionEndToEnd test
    {
        tachyon::text::FontRegistry font_registry;
        std::filesystem::path font_path = fixture_path();
        check_true(std::filesystem::exists(font_path), "Test font should exist");
        
        if (std::filesystem::exists(font_path)) {
            check_true(font_registry.load_bdf("default", font_path), "Should load test font");

            tachyon::renderer2d::RenderContext2D context;
            context.font_registry = &font_registry;

            tachyon::scene::EvaluatedLayerState layer;
            layer.type = tachyon::scene::LayerType::Text;
            layer.is_3d = true;
            layer.visible = true;
            layer.enabled = true;
            layer.active = true;
            layer.text_content = "Hello";
            layer.font_id = "default";
            layer.font_size = 14.0f;
            layer.world_matrix = tachyon::math::Matrix4x4::identity();
            layer.previous_world_matrix = tachyon::math::Matrix4x4::identity();
            layer.width = 200;
            layer.height = 50;
            layer.fill_color = {255, 255, 255, 255};
            layer.opacity = 1.0f;

            tachyon::scene::EvaluatedCompositionState state;
            state.layers.push_back(layer);

            tachyon::Scene3DBridgeInput input;
            input.state = &state;
            input.context = &context;
            input.plan = nullptr;
            input.task = nullptr;

            std::vector<std::size_t> block_indices = {0};
            std::vector<bool> visible_3d_layers = {true};
            input.block_indices = &block_indices;
            input.visible_3d_layers = &visible_3d_layers;

            const auto instances = tachyon::build_instances_3d(input);

            check_true(instances.size() == 1U, "Should have 1 instance for text");
            if (!instances.empty()) {
                const auto& inst = instances.front();
                check_true(inst.mesh_asset != nullptr, "Text extrusion mesh should be created");
                check_true(!inst.mesh_asset_id.empty(), "Mesh asset ID should be set");
            }
        }
    }

    return g_failures == 0;
}
