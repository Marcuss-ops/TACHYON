#include "tachyon/core/spec/schema/objects/scene_spec.h"
#include "tachyon/core/spec/schema/objects/layer_spec.h"
#include <iostream>
#include <string>

namespace tachyon {
// Forward declaration of the hash function we want to test
std::uint64_t hash_scene_content(const SceneSpec& scene);
}

namespace {
int g_failures = 0;

void check_true(bool condition, const std::string& message) {
    if (!condition) {
        ++g_failures;
        std::cerr << "FAIL: " << message << '\n';
    }
}

tachyon::SceneSpec create_base_scene() {
    using namespace tachyon;
    SceneSpec scene;
    scene.project.id = "test-project";
    
    CompositionSpec comp;
    comp.id = "main";
    comp.width = 1920;
    comp.height = 1080;
    
    LayerSpec layer;
    layer.id = "layer-1";
    layer.type = LayerType::Solid;
    
    comp.layers.push_back(layer);
    scene.compositions.push_back(comp);
    
    return scene;
}
}

bool run_scene_hash_coverage_tests() {
    using namespace tachyon;
    g_failures = 0;

    {
        auto scene = create_base_scene();
        auto hash_a = hash_scene_content(scene);
        
        scene.compositions[0].layers[0].transition_in.transition_id = "fade-in";
        auto hash_b = hash_scene_content(scene);
        
        check_true(hash_a != hash_b, "Hash should change when transition_id changes");
    }

    {
        auto scene = create_base_scene();
        scene.compositions[0].layers[0].type = LayerType::Procedural;
        scene.compositions[0].layers[0].procedural.emplace();
        scene.compositions[0].layers[0].procedural->kind = "clouds";
        
        auto hash_a = hash_scene_content(scene);
        
        scene.compositions[0].layers[0].procedural->kind = "fire";
        auto hash_b = hash_scene_content(scene);
        
        check_true(hash_a != hash_b, "Hash should change when procedural kind changes");
    }

    {
        auto scene = create_base_scene();
        auto hash_a = hash_scene_content(scene);
        
        scene.compositions[0].layers[0].camera2d_id = "main-camera";
        auto hash_b = hash_scene_content(scene);
        
        check_true(hash_a != hash_b, "Hash should change when camera2d_id changes");
    }

    {
        auto scene = create_base_scene();
        auto hash_a = hash_scene_content(scene);
        
        scene.compositions[0].layers[0].timing.start = 5.0;
        auto hash_b = hash_scene_content(scene);
        
        check_true(hash_a != hash_b, "Hash should change when layer start time changes");
    }

    {
        auto scene = create_base_scene();
        auto hash_a = hash_scene_content(scene);
        
        scene.compositions[0].layers[0].transform.position_x = 100.0;
        auto hash_b = hash_scene_content(scene);
        
        check_true(hash_a != hash_b, "Hash should change when transform position changes");
    }

    {
        auto scene = create_base_scene();
        auto hash_a = hash_scene_content(scene);
        
        scene.compositions[0].layers[0].transform.position_property.keyframes.push_back({0.0, math::Vector2{0,0}});
        auto hash_b = hash_scene_content(scene);
        
        check_true(hash_a != hash_b, "Hash should change when transform keyframes are added");
    }

    return g_failures == 0;
}
