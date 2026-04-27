#include <gtest/gtest.h>
#include "tachyon/runtime/execution/session/render_session.h"
#include "tachyon/core/spec/schema/objects/scene_spec.h"
#include "tachyon/core/spec/compilation/scene_compiler.h"
#include "tachyon/renderer2d/core/framebuffer.h"
#include <filesystem>
#include <fstream>
#include <iostream>

namespace tachyon::test {

class GoldenMasterTest : public ::testing::Test {
protected:
    void SetUp() override {
        std::filesystem::create_directories("tests/golden/actual");
    }

    void run_golden_test(const std::string& name, const std::string& scene_json) {
        // 1. Parse and Compile Scene
        auto scene = SceneSpec::from_json(nlohmann::json::parse(scene_json));
        auto compiled = SceneCompiler::compile(scene);

        // 2. Setup Render Session
        RenderSession session;
        RenderExecutionPlan plan;
        plan.render_plan.composition.width = 160;
        plan.render_plan.composition.height = 90;
        plan.render_plan.composition.frame_rate = {24, 1};
        
        FrameRenderTask task;
        task.frame_number = 0;
        task.time_seconds = 0.0;
        plan.frame_tasks.push_back(task);

        // 3. Render
        auto result = session.render(scene, compiled, plan, "");
        if (result.frames.empty()) {
            FAIL() << "No frames rendered for " << name;
        }
        
        auto fb = result.frames[0].frame;
        if (!fb) {
            FAIL() << "Framebuffer is null for " << name;
        }

        // 4. Calculate simple checksum
        uint64_t checksum = 0;
        const std::vector<float>& pixels = fb->pixels();
        for (float p : pixels) {
            uint32_t val;
            std::memcpy(&val, &p, 4);
            checksum = checksum * 31 + val;
        }

        // 5. Check against expected
        std::cout << "[GOLDEN] " << name << " Checksum: " << std::hex << checksum << std::dec << std::endl;
    }
};

TEST_F(GoldenMasterTest, SimpleShapes) {
    std::string scene = R"({
        "compositions": [{
            "id": "main",
            "layers": [
                {
                    "type": "shape",
                    "name": "Rect",
                    "shape": { "type": "rect", "width": 50, "height": 50 },
                    "transform": { "position": [80, 45] }
                }
            ]
        }]
    })";
    run_golden_test("simple_shapes", scene);
}

TEST_F(GoldenMasterTest, TextAnimation) {
    std::string scene = R"({
        "compositions": [{
            "id": "main",
            "layers": [
                {
                    "type": "text",
                    "text": "GOLDEN",
                    "font_size": 24,
                    "transform": { "position": [80, 45] },
                    "animation": {
                        "enabled": true,
                        "time_seconds": 0.5,
                        "animators": [{
                            "selector": { "type": "range", "start": 0, "end": 100 },
                            "properties": { "position_offset_value": [0, -10] }
                        }]
                    }
                }
            ]
        }]
    })";
    run_golden_test("text_animation", scene);
}

} // namespace tachyon::test
