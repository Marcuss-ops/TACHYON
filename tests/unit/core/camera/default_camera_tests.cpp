#include "tachyon/core/scene/evaluation/evaluator.h"
#include "tachyon/core/scene/state/evaluated_state.h"
#include "tachyon/core/math/vector2.h"
#include "tachyon/core/math/vector3.h"
#include <iostream>
#include <string>

namespace {

int g_failures = 0;

void check_true(bool condition, const std::string& message) {
    if (!condition) {
        ++g_failures;
        std::cerr << "FAIL: " << message << '\n';
    } else {
        std::cout << "PASS: " << message << '\n';
    }
}

bool nearly_equal(float a, float b, float eps = 1e-3f) {
    return std::abs(a - b) <= eps;
}

} // namespace

void run_default_camera_tests() {
    using namespace tachyon;
    using namespace tachyon::scene;

    std::cout << "\n--- Running Default Camera Contract Tests ---\n";

    CompositionSpec comp;
    comp.width = 1920;
    comp.height = 1080;
    comp.frame_rate = {30, 1};

    std::vector<EvaluatedLayerState> layers; // No camera layer

    EvaluatedCameraState camera = evaluate_camera_state(comp, layers, 0, 0.0);

    // 1. Check if camera is available (it should be false if using default, but validly populated)
    check_true(!camera.available, "Default camera should be marked as not available (explicitly)");
    
    // 2. Check position
    check_true(nearly_equal(camera.position.x, 960.0f), "Default camera X should be at comp center");
    check_true(nearly_equal(camera.position.y, 540.0f), "Default camera Y should be at comp center");
    check_true(camera.position.z < 0.0f, "Default camera Z should be negative (looking at z=0)");

    // 3. Check POI/Target
    check_true(nearly_equal(camera.point_of_interest.x, 960.0f), "Default camera Target X should be at comp center");
    check_true(nearly_equal(camera.point_of_interest.y, 540.0f), "Default camera Target Y should be at comp center");
    check_true(nearly_equal(camera.point_of_interest.z, 0.0f), "Default camera Target Z should be at z=0");

    // 4. Verify the "2D Parity" contract
    // A point at (0, 0, 0) should project to (0, 0) in screen space? 
    // Actually, (0, 0, 0) in world is top-left in AE? 
    // Wait, let's see how project_point works.
    
    math::Vector3 top_left_world{0.0f, 0.0f, 0.0f};
    math::Vector3 bottom_right_world{1920.0f, 1080.0f, 0.0f};

    // We can use the camera.camera (backward compat struct) to project
    math::Vector2 top_left_proj = camera.camera.project_point(top_left_world, 1920.0f, 1080.0f);
    math::Vector2 bottom_right_proj = camera.camera.project_point(bottom_right_world, 1920.0f, 1080.0f);


    check_true(nearly_equal(top_left_proj.x, 0.0f) && nearly_equal(top_left_proj.y, 0.0f), 
        "World (0,0,0) should project to Screen (0,0) with Default Camera");
    check_true(nearly_equal(bottom_right_proj.x, 1920.0f) && nearly_equal(bottom_right_proj.y, 1080.0f), 
        "World (1920,1080,0) should project to Screen (1920,1080) with Default Camera");

    if (g_failures > 0) {
        std::cerr << "Default Camera Tests failed with " << g_failures << " errors.\n";
        exit(1);
    }
}

void run_parallax_tests() {
    using namespace tachyon;
    using namespace tachyon::scene;

    std::cout << "\n--- Running Parallax Tests ---\n";
    g_failures = 0;

    CompositionSpec comp;
    comp.width = 1920;
    comp.height = 1080;
    comp.frame_rate = {30, 1};

    // Camera at center, zoom 1000
    LayerSpec cam_layer;
    cam_layer.id = "camera";
    cam_layer.kind = LayerType::Camera;
    cam_layer.type = LayerType::Camera;
    cam_layer.transform3d.position_property.value = math::Vector3{960.0f, 540.0f, -1000.0f};
    cam_layer.camera_zoom.value = 1000.0;
    cam_layer.camera_type = "one_node";

    // 1. Test Static Parallax (different Z depths)
    std::vector<LayerSpec> spec_layers = { cam_layer };
    // We need to evaluate them to get world matrices
    // For simplicity in unit tests, we'll manually create EvaluatedLayerState
    EvaluatedLayerState cam_eval;
    cam_eval.id = "camera";
    cam_eval.type = LayerType::Camera;
    cam_eval.world_matrix = math::Matrix4x4::translation({960.0f, 540.0f, -1000.0f});
    cam_eval.camera_type = "one_node";
    cam_eval.zoom = 1000.0f;

    std::vector<EvaluatedLayerState> layers = { cam_eval };
    EvaluatedCameraState camera = evaluate_camera_state(comp, layers, 0, 0.0);

    math::Vector3 p_at_0{960.0f, 540.0f, 0.0f};
    math::Vector3 p_at_500{960.0f, 540.0f, 500.0f};

    // Move camera X by 100, but still look towards +Z
    math::Vector3 eye{1060.0f, 540.0f, -1000.0f};
    math::Vector3 target{1060.0f, 540.0f, 0.0f}; // Look straight ahead
    cam_eval.world_matrix = math::Matrix4x4::look_at(eye, target, {0.0f, -1.0f, 0.0f}).inverse_affine();
    layers[0] = cam_eval;
    camera = evaluate_camera_state(comp, layers, 0, 0.0);

    math::Vector2 proj_0 = camera.camera.project_point(p_at_0, 1920.0f, 1080.0f);
    math::Vector2 proj_500 = camera.camera.project_point(p_at_500, 1920.0f, 1080.0f);

    // With camera at +100, point at 960 (center) should appear to move LEFT by some amount.
    // Amount = shift * zoom / distance
    // At z=0, dist = 1000. shift = 100. offset = 100 * 1000 / 1000 = 100.
    // At z=500, dist = 1500. shift = 100. offset = 100 * 1000 / 1500 = 66.6.


    check_true(nearly_equal(proj_0.x, 860.0f), "Point at Z=0 should move by 100px when camera moves 100px");
    check_true(nearly_equal(proj_500.x, 893.333f), "Point at Z=500 should move less (parallax)");

    if (g_failures > 0) {
        std::cerr << "Parallax Tests failed.\n";
        exit(1);
    }
}

void run_look_at_tests() {
    using namespace tachyon;
    using namespace tachyon::scene;

    std::cout << "\n--- Running Look-At Tests ---\n";
    g_failures = 0;

    CompositionSpec comp;
    comp.width = 1920;
    comp.height = 1080;
    comp.frame_rate = {30, 1};

    EvaluatedLayerState cam_eval;
    cam_eval.id = "camera";
    cam_eval.type = LayerType::Camera;
    cam_eval.camera_type = "two_node";
    cam_eval.world_matrix = math::Matrix4x4::translation({0.0f, 0.0f, -1000.0f});
    cam_eval.poi = {1920.0f, 1080.0f, 0.0f}; // Looking at bottom-right
    cam_eval.zoom = 1000.0f;

    std::vector<EvaluatedLayerState> layers = { cam_eval };
    EvaluatedCameraState camera = evaluate_camera_state(comp, layers, 0, 0.0);

    // If looking at bottom-right, the point {1920, 1080, 0} should project to screen center {960, 540}
    math::Vector2 target_proj = camera.camera.project_point({1920.0f, 1080.0f, 0.0f}, 1920.0f, 1080.0f);
    
    check_true(nearly_equal(target_proj.x, 960.0f) && nearly_equal(target_proj.y, 540.0f), 
        "Two-node camera Target should project to screen center");

    if (g_failures > 0) {
        std::cerr << "Look-At Tests failed.\n";
        exit(1);
    }
}
