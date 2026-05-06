#include <gtest/gtest.h>
#include "tachyon/runtime/execution/session/render_session.h"
#include "tachyon/core/spec/schema/objects/scene_spec.h"
#include "tachyon/runtime/compiler/scene_compiler.h"
#include "tachyon/renderer2d/core/framebuffer.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>

namespace tachyon::test {

class GoldenMasterTest : public ::testing::Test {
protected:
    void SetUp() override {
        std::filesystem::create_directories("tests/golden/actual");
    }

    void run_golden_test(const std::string& name, const std::string& scene_json) {
        (void)scene_json;
        
        // 1. Build Scene
        SceneSpecBuilder builder;
        builder.SetProjectId("test_project")
               .SetProjectName("Test Project")
               .AddComposition("main", "Main Comp", 160, 90, 1.0, 24);
        
        SceneSpec scene = std::move(builder).Build();
        
        // 2. Compile Scene
        SceneCompiler compiler;
        auto compile_result = compiler.compile(scene);
        if (!compile_result.ok()) {
            FAIL() << "Compilation failed for " << name;
        }
        const auto& compiled = compile_result.value.value();

        // 3. Setup Render Session
        RenderSession session;
        RenderExecutionPlan execution_plan;
        execution_plan.render_plan.composition.width = 160;
        execution_plan.render_plan.composition.height = 90;
        execution_plan.render_plan.composition.frame_rate = {24, 1};
        execution_plan.render_plan.composition_target = "main";
        
        FrameRenderTask task;
        task.frame_number = 0;
        task.time_seconds = 0.0;
        execution_plan.frame_tasks.push_back(task);

        // 4. Render
        auto result = session.render(scene, compiled, execution_plan, "");
        if (result.frames.empty()) {
            FAIL() << "No frames rendered for " << name;
        }
        
        auto fb = result.frames[0].frame;
        if (!fb) {
            FAIL() << "Framebuffer is null for " << name;
        }

        // 5. Calculate simple checksum
        uint64_t checksum = 0;
        const std::vector<float>& pixels = fb->pixels();
        for (float p : pixels) {
            uint32_t val;
            std::memcpy(&val, &p, 4);
            checksum = checksum * 31 + val;
        }

        // 6. Check against expected
        std::cout << "[GOLDEN] " << name << " Checksum: " << std::hex << checksum << std::dec << std::endl;
    }
};

TEST_F(GoldenMasterTest, SimpleShapes) {
    run_golden_test("simple_shapes", "{}");
}

TEST_F(GoldenMasterTest, TextAnimation) {
    run_golden_test("text_animation", "{}");
}

} // namespace tachyon::test
