#include "gtest/gtest.h"
#include "tachyon/runtime/execution/session/render_session.h"
#include "tachyon/runtime/execution/frames/frame_hasher.h"
#include "tachyon/core/spec/compilation/scene_compiler.h"

namespace tachyon {

class DeterminismTests : public ::testing::Test {
protected:
    void SetUp() override {}

    SceneSpec create_complex_test_scene() {
        SceneSpec scene;
        scene.compositions.push_back({});
        auto& comp = scene.compositions.back();
        comp.id = "main";
        comp.width = 1280;
        comp.height = 720;
        comp.duration = 10.0;

        for (int i = 0; i < 10; ++i) {
            comp.layers.push_back({});
            auto& layer = comp.layers.back();
            layer.id = "L" + std::to_string(i);
            layer.type = LayerType::Solid;
            layer.transform.position_x = 100.0 * i;
            layer.transform.position_y = 50.0 * i;
            layer.opacity = 0.5 + (0.05 * i);
        }
        return scene;
    }

    SceneSpec create_nested_test_scene() {
        SceneSpec scene;
        
        // Child
        scene.compositions.push_back({});
        auto& child = scene.compositions.back();
        child.id = "child";
        child.width = 640;
        child.height = 360;
        child.layers.push_back({});
        child.layers.back().id = "C1";
        child.layers.back().type = LayerType::Solid;
        child.layers.back().transform.position_x = 320;
        child.layers.back().transform.position_y = 180;

        // Parent
        scene.compositions.push_back({});
        auto& parent = scene.compositions.back();
        parent.id = "parent";
        parent.width = 1280;
        parent.height = 720;
        parent.layers.push_back({});
        auto& l1 = parent.layers.back();
        l1.id = "L1";
        l1.type = LayerType::Precomp;
        l1.precomp_id = "child";
        l1.transform.position_x = 100;
        l1.transform.position_y = 100;

        return scene;
    }
};

TEST_F(DeterminismTests, SequentialVsParallelBitIdentity) {
    auto scene = create_complex_test_scene();
    
    SceneCompiler compiler({});
    auto compiled = *compiler.compile(scene).value;

    RenderExecutionPlan execution_plan;
    execution_plan.render_plan.quality_tier = "production";
    for (int i = 0; i < 5; ++i) {
        execution_plan.frame_tasks.push_back({static_cast<std::int64_t>(i), {}});
    }

    // 1. Sequential Render
    RenderSession seq_session;
    auto seq_result = seq_session.render(scene, compiled, execution_plan, "", 1);

    // 2. Parallel Render (with different worker counts)
    std::vector<int> worker_counts = {2, 4, 8, 16};
    FrameHasher hasher;

    for (int workers : worker_counts) {
        RenderSession par_session;
        auto par_result = par_session.render(scene, compiled, execution_plan, "", workers);

        ASSERT_EQ(seq_result.frames.size(), par_result.frames.size());

        for (size_t i = 0; i < seq_result.frames.size(); ++i) {
            const auto& f1 = seq_result.frames[i];
            const auto& f2 = par_result.frames[i];
            
            const std::uint64_t h1 = hasher.hash_pixels(f1.frame->pixels());
            const std::uint64_t h2 = hasher.hash_pixels(f2.frame->pixels());

            EXPECT_EQ(h1, h2) << "Deterministic failure at frame " << i << " with " << workers << " workers";
            EXPECT_EQ(f1.cache_key, f2.cache_key) << "Cache key mismatch at frame " << i << " with " << workers << " workers";
        }
    }
}

TEST_F(DeterminismTests, NestedCompositionIdentity) {
    auto scene = create_nested_test_scene();
    
    SceneCompiler compiler({});
    auto compiled = *compiler.compile(scene).value;

    RenderExecutionPlan execution_plan;
    execution_plan.frame_tasks.push_back({0, {}});

    RenderSession seq_session;
    auto seq_result = seq_session.render(scene, compiled, execution_plan, "", 1);

    RenderSession par_session;
    auto par_result = par_session.render(scene, compiled, execution_plan, "", 4);

    FrameHasher hasher;
    std::uint64_t h1 = hasher.hash_pixels(seq_result.frames[0].frame->pixels());
    std::uint64_t h2 = hasher.hash_pixels(par_result.frames[0].frame->pixels());

    EXPECT_EQ(h1, h2);
}

} // namespace tachyon
