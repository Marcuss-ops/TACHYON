#include "tachyon/core/render/scene_3d.h"
#include "tachyon/core/scene/state/evaluated_state.h"
#include "tachyon/renderer2d/evaluated_composition/rendering/pipeline/scene3d_bridge.h"
#include "tachyon/runtime/execution/planning/render_plan.h"
#include "tachyon/scene/builder.h"

#include <cmath>
#include <iostream>
#include <vector>
#include <string>

namespace {

int g_failures = 0;

void check_true(bool condition, const std::string& message) {
    if (!condition) {
        ++g_failures;
        std::cerr << "FAIL: " << message << '\n';
    }
}

bool nearly_equal(float a, float b, float epsilon = 1.0e-4f) {
    return std::abs(a - b) <= epsilon;
}

tachyon::scene::EvaluatedLayerState make_basic_layer(
    const std::string& id,
    tachyon::LayerType type,
    bool is_3d,
    int width,
    int height) {

    tachyon::scene::EvaluatedLayerState layer;
    layer.id = id;
    layer.layer_id = id;
    layer.name = id;
    layer.type = type;
    layer.is_3d = is_3d;
    layer.enabled = true;
    layer.visible = true;
    layer.active = true;
    layer.width = width;
    layer.height = height;
    layer.world_matrix = tachyon::math::Matrix4x4::identity();
    layer.previous_world_matrix = layer.world_matrix;
    return layer;
}

}  // namespace

bool run_2d_3d_bridge_tests() {
    g_failures = 0;

    using namespace tachyon;

    // Test 1: Build camera from scene spec when evaluated camera is unavailable.
    {
        const auto scene = scene::Composition("main")
            .size(1920, 1080)
            .camera3d_layer("cam", [](scene::LayerBuilder& l) {
                l.position3d(10.0, 20.0, -1000.0)
                 .camera().type("two_node").poi(0.0, 0.0, 0.0).zoom(877.0).done();
            })
            .build_scene();

        scene::EvaluatedCompositionState state;
        state.width = 1920;
        state.height = 1080;

        RenderPlan plan;
        plan.scene_spec = &scene;
        plan.composition_target = "main";

        Scene3DBridgeInput input;
        input.state = &state;
        input.plan = &plan;
        const std::vector<std::size_t> block_indices;
        const std::vector<bool> visible_3d_layers;
        input.block_indices = &block_indices;
        input.visible_3d_layers = &visible_3d_layers;

        const auto output = build_evaluated_scene_3d(input);
        check_true(output.scene3d.camera.camera_id == "cam",
            "Bridge should pick the camera layer from the scene spec");
        check_true(nearly_equal(output.scene3d.camera.position.x, 10.0f) &&
                   nearly_equal(output.scene3d.camera.position.y, 20.0f) &&
                   nearly_equal(output.scene3d.camera.position.z, -1000.0f),
            "Camera position should come from the camera layer transform");
        check_true(nearly_equal(output.scene3d.camera.target.x, 0.0f) &&
                   nearly_equal(output.scene3d.camera.target.y, 0.0f) &&
                   nearly_equal(output.scene3d.camera.target.z, 0.0f),
            "Camera target should come from the camera point of interest");
        check_true(output.scene3d.camera.is_active_camera,
            "Camera from spec should be marked active");
    }

    // Test 2: Fall back to the default perspective camera when no camera exists.
    {
        scene::EvaluatedCompositionState state;
        state.width = 1280;
        state.height = 720;

        Scene3DBridgeInput input;
        input.state = &state;
        const std::vector<std::size_t> block_indices;
        const std::vector<bool> visible_3d_layers;
        input.block_indices = &block_indices;
        input.visible_3d_layers = &visible_3d_layers;

        const auto output = build_evaluated_scene_3d(input);
        check_true(output.scene3d.camera.camera_id == "default_perspective",
            "Bridge should synthesize a default camera when none is provided");
        check_true(output.scene3d.camera.position.z < 0.0f,
            "Default camera should be placed in front of the scene");
        check_true(output.scene3d.camera.target.x != 0.0f || output.scene3d.camera.target.y != 0.0f,
            "Default camera target should not be the origin");
    }

    // Test 3: Renderable 3D layers should survive the bridge while camera/light/null are skipped.
    {
        scene::EvaluatedCompositionState state;
        state.width = 1920;
        state.height = 1080;
        state.layers = {
            make_basic_layer("cam", LayerType::Camera, true, 0, 0),
            make_basic_layer("key", LayerType::Light, true, 0, 0),
            make_basic_layer("null", LayerType::NullLayer, true, 0, 0),
            make_basic_layer("plate", LayerType::Solid, true, 240, 120)
        };
        state.layers[0].camera_type = "two_node";
        state.layers[0].poi = math::Vector3{0.0f, 0.0f, 0.0f};
        state.layers[0].world_position3 = math::Vector3{10.0f, 20.0f, -1000.0f};
        state.layers[3].fill_color = ColorSpec{200, 30, 40, 255};

        std::vector<std::size_t> block_indices = {0, 1, 2, 3};
        std::vector<bool> visible_3d_layers = {true, true, true, true};

        Scene3DBridgeInput input;
        input.state = &state;
        input.block_indices = &block_indices;
        input.visible_3d_layers = &visible_3d_layers;

        const auto instances = build_instances_3d(input);
        check_true(instances.size() == 2,
            "Only the floor grid and the solid layer should produce 3D instances");

        bool saw_floor_grid = false;
        bool saw_plate = false;
        for (const auto& instance : instances) {
            if (instance.mesh_asset_id == "floor_grid") {
                saw_floor_grid = true;
                continue;
            }

            if (instance.mesh_asset_id.find("card3d:solid:plate:0") == 0) {
                saw_plate = true;
                check_true(instance.material.base_color == ColorSpec{200, 30, 40, 255},
                    "Solid layer color should propagate into the 3D material");
                continue;
            }

            check_true(false,
                "Camera, light, and null layers must not become 3D mesh instances");
        }

        check_true(saw_floor_grid, "Bridge should add the floor grid when a visible 3D layer exists");
        check_true(saw_plate, "Solid layer should become a 3D card instance");
    }

    std::cout << "2D/3D bridge tests: " << (g_failures == 0 ? "PASSED" : "FAILED")
              << " (" << g_failures << " failures)\n";

    return g_failures == 0;
}
