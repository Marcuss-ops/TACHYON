#include "tachyon/core/spec/schema/objects/layer_spec.h"
#include "tachyon/runtime/execution/planning/hash/scene_hash.h"
#include <gtest/gtest.h>

namespace tachyon::test {

TEST(SceneHashCoverageTest, TimingChangesHash) {
    SceneSpec scene;
    scene.compositions.emplace_back();
    scene.compositions[0].layers.emplace_back();
    scene.compositions[0].layers[0].id = "layer1";
    scene.compositions[0].layers[0].type = LayerType::Image;

    auto hash_a = hash_scene_content(scene);
    
    scene.compositions[0].layers[0].timing.duration = 4.0;
    auto hash_b = hash_scene_content(scene);
    
    EXPECT_NE(hash_a, hash_b) << "Hash must change when timing duration changes";
    
    scene.compositions[0].layers[0].timing.start = 1.0;
    auto hash_c = hash_scene_content(scene);
    EXPECT_NE(hash_b, hash_c) << "Hash must change when timing start changes";
}

TEST(SceneHashCoverageTest, TransformChangesHash) {
    SceneSpec scene;
    scene.compositions.emplace_back();
    scene.compositions[0].layers.emplace_back();
    scene.compositions[0].layers[0].id = "layer1";
    scene.compositions[0].layers[0].type = LayerType::Image;

    auto hash_a = hash_scene_content(scene);
    
    scene.compositions[0].layers[0].transform.position_x = 100.0;
    auto hash_b = hash_scene_content(scene);
    
    EXPECT_NE(hash_a, hash_b) << "Hash must change when transform position changes";
}

} // namespace tachyon::test
