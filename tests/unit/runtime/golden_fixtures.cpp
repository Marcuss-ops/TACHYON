#include "gtest/gtest.h"
#include "tachyon/runtime/execution/frame_executor.h"
#include "tachyon/runtime/execution/frame_hasher.h"
#include "tachyon/core/spec/scene_compiler.h"
#include "golden_registry.h"

namespace tachyon {

class GoldenFixtureTests : public ::testing::Test {
protected:
    void SetUp() override {
        m_registry = std::make_unique<test::GoldenRegistry>("golden_hashes.json");
    }

    void verify_hash(const std::string& key, const ExecutedFrame& result) {
        FrameHasher hasher;
        std::uint64_t hash = 0;
        if (result.frame.surface) {
            hash = hasher.hash_pixels(result.frame.surface->pixels());
        }

        // To update hashes, change false to true here during development
        bool update_mode = false; 
        bool success = m_registry->verify_or_update(key, hash, update_mode);
        
        EXPECT_TRUE(success) << "Hash mismatch for " << key << ". Calculated: 0x" << std::hex << hash;
    }

    std::unique_ptr<test::GoldenRegistry> m_registry;
};

TEST_F(GoldenFixtureTests, SimpleTransformBitIdentity) {
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

    SceneCompiler compiler({});
    auto compiled = compiler.compile(scene).value;

    FrameCache cache;
    FrameArena arena;
    FrameExecutor executor(arena, cache);
    RenderContext context;
    RenderPlan plan;
    FrameRenderTask task{0, {}};
    
    auto result = executor.execute(compiled, plan, task, context);
    verify_hash("simple_transform", result);
}

TEST_F(GoldenFixtureTests, NestedPrecompComplex) {
    SceneSpec scene;
    
    // Child
    scene.compositions.push_back({});
    auto& child = scene.compositions.back();
    child.id = "child";
    child.width = 500;
    child.height = 500;
    child.layers.push_back({});
    child.layers.back().id = "C1";
    child.layers.back().type = "solid";
    child.layers.back().opacity = 0.5;
    
    // Parent
    scene.compositions.push_back({});
    auto& root = scene.compositions.back();
    root.id = "root";
    root.width = 1920;
    root.height = 1080;
    
    root.layers.push_back({});
    auto& p1 = root.layers.back();
    p1.id = "P1";
    p1.type = "precomp";
    p1.precomp_id = "child";
    p1.transform.position_x = 200;

    SceneCompiler compiler({});
    auto compiled = compiler.compile(scene).value;

    FrameCache cache;
    FrameArena arena;
    FrameExecutor executor(arena, cache);
    RenderContext context;
    RenderPlan plan;
    FrameRenderTask task{0, {}};
    
    auto result = executor.execute(compiled, plan, task, context);
    verify_hash("nested_precomp_complex", result);
}

TEST_F(GoldenFixtureTests, BlendingModeHardening) {
    SceneSpec scene;
    scene.compositions.push_back({});
    auto& comp = scene.compositions.back();
    comp.id = "root";
    comp.width = 1280;
    comp.height = 720;

    // Layer 1: Base
    comp.layers.push_back({});
    comp.layers.back().id = "base";
    comp.layers.back().type = "solid";

    // Layer 2: Additive blend
    comp.layers.push_back({});
    auto& l2 = comp.layers.back();
    l2.id = "add";
    l2.type = "solid";
    l2.blend_mode = "add";
    l2.opacity = 0.5;

    SceneCompiler compiler({});
    auto compiled = compiler.compile(scene).value;

    FrameCache cache;
    FrameArena arena;
    FrameExecutor executor(arena, cache);
    RenderContext context;
    RenderPlan plan;
    FrameRenderTask task{0, {}};
    
    auto result = executor.execute(compiled, plan, task, context);
    verify_hash("blending_mode_hardening", result);
}

} // namespace tachyon
