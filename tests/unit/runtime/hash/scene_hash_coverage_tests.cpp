#include "test_utils.h"
#include "tachyon/core/spec/schema/objects/layer_spec.h"
#include "tachyon/runtime/execution/planning/hash/scene_hash.h"
#include <iostream>

namespace tachyon::test {

bool check_timing_hash() {
    SceneSpec scene;
    scene.compositions.emplace_back();
    scene.compositions[0].id = "main";
    scene.compositions[0].layers.emplace_back();
    scene.compositions[0].layers[0].identity.id = "layer1";
    scene.compositions[0].layers[0].identity.type = LayerType::Image;

    auto hash_a = hash_scene_content(scene);
    
    scene.compositions[0].layers[0].playback.timing.duration = 4.0;
    auto hash_b = hash_scene_content(scene);
    
    if (hash_a == hash_b) {
        std::cerr << "FAIL: Hash must change when timing duration changes\n";
        return false;
    }
    
    scene.compositions[0].layers[0].playback.timing.start = 1.0;
    auto hash_c = hash_scene_content(scene);
    if (hash_b == hash_c) {
        std::cerr << "FAIL: Hash must change when timing start changes\n";
        return false;
    }
    return true;
}

bool check_transform_hash() {
    SceneSpec scene;
    scene.compositions.emplace_back();
    scene.compositions[0].id = "main";
    scene.compositions[0].layers.emplace_back();
    scene.compositions[0].layers[0].identity.id = "layer1";
    scene.compositions[0].layers[0].identity.type = LayerType::Image;

    auto hash_a = hash_scene_content(scene);
    
    scene.compositions[0].layers[0].transform.transform.position_x = 100.0;
    auto hash_b = hash_scene_content(scene);
    
    if (hash_a == hash_b) {
        std::cerr << "FAIL: Hash must change when transform position changes\n";
        return false;
    }
    return true;
}

} // namespace tachyon::test

bool run_scene_hash_coverage_tests() {
    bool ok = true;
    ok &= tachyon::test::check_timing_hash();
    ok &= tachyon::test::check_transform_hash();
    return ok;
}
