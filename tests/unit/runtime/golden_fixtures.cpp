#include "gtest/gtest.h"
#include "tachyon/runtime/execution/frame_executor.h"
#include "tachyon/runtime/execution/frame_hasher.h"
#include "tachyon/core/spec/scene_compiler.h"

namespace tachyon {

class GoldenFixtureTests : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize common test environment
    }
};

TEST_F(GoldenFixtureTests, SimpleTransformBitIdentity) {
    // 1. Create a simple scene (authoring)
    SceneSpec scene;
    scene.compositions.push_back({});
    auto& comp = scene.compositions.back();
    comp.id = "root";
    comp.width = 1920;
    comp.height = 1080;
    comp.duration = 1.0;
    comp.layers.push_back({});
    auto& layer = comp.layers.back();
    layer.id = "L1";
    layer.type = "solid";
    layer.transform.position_x = 100.0;
    layer.transform.position_y = 200.0;

    // 2. Compile once
    SceneCompiler compiler({});
    auto compiled = compiler.compile(scene).value;

    // 3. Execute
    FrameCache cache;
    FrameArena arena;
    FrameExecutor executor(arena, cache);
    RenderContext context;
    
    RenderPlan plan;
    plan.quality_tier = "production";
    FrameRenderTask task{0, {}};
    
    auto result = executor.execute(compiled, plan, task, context);

    // 4. Verify Bit-Identity via Cryptographic Hashing
    FrameHasher hasher;
    std::uint64_t hash = 0;
    if (result.frame.surface.has_value()) {
        hash = hasher.hash_pixels(result.frame.surface->pixels());
    }

    // This is the 'Golden' hash for this specific scene on a standard float build
    const std::uint64_t kGoldenHash = 0x8df5a122340b1922ULL; 
    
    EXPECT_EQ(hash, kGoldenHash);
    EXPECT_EQ(result.scene_hash, compiled.scene_hash);
}


} // namespace tachyon
