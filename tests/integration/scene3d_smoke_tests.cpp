#include "tachyon/scene/builder.h"
#include "tachyon/core/scene/evaluation/evaluator.h"
#include "tachyon/core/scene/state/evaluated_state.h"
#include "tachyon/renderer2d/evaluated_composition/rendering/pipeline/scene3d_bridge.h"
#include "tachyon/renderer3d/core/ray_tracer.h"
#include "tachyon/media/management/media_manager.h"
#include "tachyon/media/loading/mesh_asset.h"
#include "tachyon/core/math/algebra/vector3.h"

#include <memory>
#include <vector>
#include <string>
#include <iostream>
#include <cassert>

namespace {

int g_failures = 0;

void check_true(bool condition, const std::string& message) {
    if (!condition) {
        ++g_failures;
        std::cerr << "FAIL: " << message << '\n';
    }
}

// Create a simple procedural cube mesh
std::unique_ptr<tachyon::media::MeshAsset> create_test_cube() {
    auto asset = std::make_unique<tachyon::media::MeshAsset>();
    
    tachyon::media::MeshAsset::SubMesh cube;
    
    // Cube vertices (position, normal, uv)
    struct Vert { float x, y, z, nx, ny, nz, u, v; };
    Vert vertices[] = {
        // Front face
        {-0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f},
        {0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f},
        {0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f},
        {-0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f},
        // Back face
        {0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f},
        {-0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f},
        {-0.5f, 0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f},
        {0.5f, 0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f},
    };
    
    for (const auto& v : vertices) {
        tachyon::media::MeshAsset::Vertex vert;
        vert.position = {v.x, v.y, v.z};
        vert.normal = {v.nx, v.ny, v.nz};
        vert.uv = {v.u, v.v};
        cube.vertices.push_back(vert);
    }
    
    // Indices for 6 faces (2 triangles each)
    cube.indices = {
        0, 1, 2, 0, 2, 3,  // Front
        4, 5, 6, 4, 6, 7,  // Back
        1, 4, 7, 1, 7, 2,  // Right
        5, 0, 3, 5, 3, 6,  // Left
        3, 2, 7, 3, 7, 6,  // Top
        5, 4, 1, 5, 1, 0   // Bottom
    };
    
    cube.transform = tachyon::math::Matrix4x4::identity();
    cube.material.base_color_factor = {0.8f, 0.8f, 0.8f};
    cube.material.metallic_factor = 0.0f;
    cube.material.roughness_factor = 0.5f;
    
    asset->sub_meshes.push_back(std::move(cube));
    return asset;
}

} // namespace

bool run_scene3d_smoke_tests() {
    g_failures = 0;
    
    std::cout << "Running 3D Scene Smoke Tests...\n";
    
    // Test 1: Build scene with mesh layer using builder API
    {
        std::cout << "Test: Build scene with 3D layers...\n";
        
        auto scene = tachyon::scene::Scene()
            .project("smoke_test", "3D Smoke Test")
            .composition("main", [](auto& comp) {
                comp.size(256, 256)
                    .fps(30)
                    .duration(1.0)
                    .clear({10, 12, 18, 255});
                
                comp.camera3d_layer("cam", [](auto& l) {
                    l.position3d(0, 0, -5)
                     .camera_poi(0, 0, 0)
                     .camera_zoom(35);
                });
                
                comp.light_layer("key", [](auto& l) {
                    l.light_type("point")
                     .position3d(2, 3, -4)
                     .light_intensity(8.0)
                     .light_color({255, 240, 220, 255});
                });
                
                comp.mesh_layer("cube", [](auto& l) {
                    l.mesh("test_cube")
                     .position3d(0, 0, 0)
                     .rotation3d(0, 30, 0)
                     .scale3d(1, 1, 1)
                     .material()
                        .metallic(0.1)
                        .roughness(0.5)
                        .done();
                });
            })
            .build();
        
        check_true(scene.compositions.size() == 1, "Should have 1 composition");
        check_true(scene.project.id == "smoke_test", "Project ID should match");
        
        const auto& comp = scene.compositions[0];
        check_true(comp.layers.size() == 3, "Should have 3 layers (camera, light, mesh)");
        
        // Check camera layer
        const auto& camera_layer = comp.layers[0];
        check_true(camera_layer.type == tachyon::LayerType::Camera, "First layer should be camera");
        check_true(camera_layer.is_3d, "Camera should be 3D");
        
        // Check light layer  
        const auto& light_layer = comp.layers[1];
        check_true(light_layer.type == tachyon::LayerType::Light, "Second layer should be light");
        check_true(light_layer.is_3d, "Light should be 3D");
        
        // Check mesh layer
        const auto& mesh_layer = comp.layers[2];
        check_true(mesh_layer.type == tachyon::LayerType::Shape, "Third layer should be Shape");
        check_true(mesh_layer.is_3d, "Mesh should be 3D");
        check_true(mesh_layer.asset_id == "test_cube", "Mesh asset_id should be 'test_cube'");
        
        std::cout << "  Scene building: PASS\n";
    }
    
    // Test 2: Evaluate composition state
    {
        std::cout << "Test: Evaluate 3D composition state...\n";
        
        auto scene = tachyon::scene::Scene()
            .project("eval_test", "Eval Test")
            .composition("main", [](auto& comp) {
                comp.size(256, 256).fps(30).duration(1.0);
                
                comp.camera3d_layer("cam", [](auto& l) {
                    l.position3d(0, 0, -5).camera_poi(0, 0, 0);
                });
                
                comp.mesh_layer("cube", [](auto& l) {
                    l.mesh("test_cube").position3d(0, 0, 0);
                });
            })
            .build();
        
        const auto& comp = scene.compositions[0];
        const auto evaluated = tachyon::scene::evaluate_composition_state(comp, static_cast<std::int64_t>(30));
        
        check_true(evaluated.layers.size() == 2, "Should have 2 evaluated layers");
        check_true(evaluated.camera.available, "Camera should be available");
        
        std::cout << "  Evaluation: PASS\n";
    }
    
    std::cout << "3D Scene Smoke Tests " << (g_failures == 0 ? "PASSED" : "FAILED") << "\n";
    return g_failures == 0;
}

// No main here, called from test_main.cpp
