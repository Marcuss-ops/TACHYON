#include "visual_golden_verifier.h"
#include "tachyon/core/spec/scene_compiler.h"
#include "tachyon/runtime/execution/frame_executor.h"
#include <iostream>

namespace {
using namespace tachyon;
using namespace tachyon::test;

bool run_canonical_scene(const std::string& scene_id, const SceneSpec& scene, uint32_t frame_index) {
    const std::filesystem::path golden_root = std::filesystem::path(TACHYON_TESTS_SOURCE_DIR) / "golden";
    VisualGoldenVerifier verifier(golden_root);

    SceneCompiler compiler({});
    auto compiled_result = compiler.compile(scene);
    if (!compiled_result.ok()) {
        std::cerr << "FAIL: " << scene_id << " compilation failed\n";
        return false;
    }
    const auto& compiled = *compiled_result.value;

    FrameCache cache;
    FrameArena arena;
    FrameExecutor executor(arena, cache);
    RenderContext context;
    RenderPlan plan;
    FrameRenderTask task{frame_index, {}};
    
    auto result = executor.execute(compiled, plan, task, context);
    if (!result.frame) {
        std::cerr << "FAIL: " << scene_id << " render produced no surface\n";
        return false;
    }

    // Toggle this to true to update golden baselines
    bool update_mode = (std::getenv("TACHYON_UPDATE_GOLDEN") != nullptr);

    if (!verifier.verify_frame(scene_id, frame_index, *result.frame, update_mode)) {
        std::cerr << "FAIL: " << verifier.last_error() << "\n";
        return false;
    }

    return true;
}

} // namespace

bool run_golden_visual_tests() {
    bool all_ok = true;

    // 1. Simple Transform
    {
        SceneSpec scene;
        scene.compositions.push_back({});
        auto& comp = scene.compositions.back();
        comp.id = "root"; comp.width = 1280; comp.height = 720; comp.duration = 1.0;
        comp.layers.push_back({});
        auto& l1 = comp.layers.back();
        l1.id = "L1"; l1.type = "solid";
        l1.transform.position_x = 640.0;
        l1.transform.position_y = 360.0;
        l1.transform.scale_x = 2.0; l1.transform.scale_y = 2.0;
        
        all_ok &= run_canonical_scene("canonical_simple_transform", scene, 0);
    }

    // 2. Nested Precomp with Opacity
    {
        SceneSpec scene;
        // Child
        scene.compositions.push_back({});
        auto& child = scene.compositions.back();
        child.id = "child"; child.width = 400; child.height = 400;
        child.layers.push_back({});
        child.layers.back().id = "C1"; child.layers.back().type = "solid";
        child.layers.back().opacity = 0.5;

        // Parent
        scene.compositions.push_back({});
        auto& root = scene.compositions.back();
        root.id = "root"; root.width = 1280; root.height = 720;
        root.layers.push_back({});
        auto& p1 = root.layers.back();
        p1.id = "P1"; p1.type = "precomp"; p1.precomp_id = "child";
        p1.transform.position_x = 300; p1.transform.position_y = 300;
        p1.opacity = 0.8;

        all_ok &= run_canonical_scene("canonical_nested_precomp", scene, 0);
    }

    // 3. Expression Stagger (Hardening Verification)
    {
        SceneSpec scene;
        scene.compositions.push_back({});
        auto& comp = scene.compositions.back();
        comp.id = "root"; comp.width = 1280; comp.height = 720;
        
        for (int i = 0; i < 5; ++i) {
            comp.layers.push_back({});
            auto& l = comp.layers.back();
            l.id = "L" + std::to_string(i);
            l.type = "solid";
            l.transform.position_property.expression = "index * 100 + 100";
            l.transform.position_y = 360.0;
        }

        all_ok &= run_canonical_scene("canonical_expression_stagger", scene, 0);
    }

    // 4. Color Workflow (Linear Blending Verification)
    {
        SceneSpec scene;
        scene.compositions.push_back({});
        auto& comp = scene.compositions.back();
        comp.id = "root"; comp.width = 512; comp.height = 512;

        // Background: Mid-gray sRGB (128)
        // Expected Linear: ~0.21
        comp.layers.push_back({});
        auto& bg = comp.layers.back();
        bg.id = "BG"; bg.type = "solid";
        bg.fill_color.value = ColorSpec{128, 128, 128, 255};

        // Overlapping red: 0.5 opacity
        comp.layers.push_back({});
        auto& l1 = comp.layers.back();
        l1.id = "L1"; l1.type = "solid";
        l1.fill_color.value = ColorSpec{255, 0, 0, 255};
        l1.opacity = 0.5;
        l1.transform.position_x = 100; l1.transform.position_y = 100;
        l1.transform.scale_x = 0.5; l1.transform.scale_y = 0.5;

        all_ok &= run_canonical_scene("canonical_color_workflow", scene, 0);
    }

    return all_ok;
}
