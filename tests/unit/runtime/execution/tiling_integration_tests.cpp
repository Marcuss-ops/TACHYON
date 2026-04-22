#include "tachyon/runtime/execution/render_session.h"
#include "tachyon/runtime/execution/render_plan.h"
#include "tachyon/core/spec/scene_compiler.h"
#include "tachyon/core/spec/scene_spec.h"
#include "tachyon/renderer2d/core/framebuffer.h"

#include <iostream>
#include <string>
#include <memory>

namespace {

int g_failures = 0;

void check(bool condition, const std::string& message) {
    if (!condition) {
        ++g_failures;
        std::cerr << "FAIL: " << message << '\n';
    }
}

tachyon::SceneSpec make_solid_scene(std::int64_t width, std::int64_t height) {
    tachyon::LayerSpec bg;
    bg.id = "bg";
    bg.type = "solid";
    bg.name = "BG";
    bg.start_time = 0.0;
    bg.in_point = 0.0;
    bg.out_point = 1.0;
    bg.opacity = 1.0;
    bg.width = width;
    bg.height = height;
    bg.fill_color.value = tachyon::ColorSpec{25, 51, 77, 255}; // ~0.1, 0.2, 0.3 in uint8

    tachyon::LayerSpec red;
    red.id = "red";
    red.type = "solid";
    red.name = "Red";
    red.start_time = 0.0;
    red.in_point = 0.0;
    red.out_point = 1.0;
    red.opacity = 0.5;
    red.width = width;
    red.height = height;
    red.fill_color.value = tachyon::ColorSpec{255, 0, 0, 255};

    tachyon::CompositionSpec comp;
    comp.id = "main";
    comp.name = "Main";
    comp.width = width;
    comp.height = height;
    comp.duration = 1.0;
    comp.frame_rate = {30, 1};
    comp.layers.push_back(bg);
    comp.layers.push_back(red);

    tachyon::SceneSpec scene;
    scene.spec_version = "1.0";
    scene.project.id = "proj";
    scene.project.name = "TilingTest";
    scene.compositions.push_back(comp);
    return scene;
}

tachyon::RenderExecutionPlan make_exec_plan(std::int64_t width, std::int64_t height, int tile_size) {
    tachyon::RenderPlan plan;
    plan.job_id = "job_tiling";
    plan.composition_target = "main";
    plan.composition.id = "main";
    plan.composition.width = width;
    plan.composition.height = height;
    plan.composition.frame_rate = {30, 1};
    plan.quality_policy.tile_size = tile_size;
    plan.output.destination.path = "";

    tachyon::FrameRenderTask task;
    task.frame_number = 0;
    task.cache_key = {"frame:0"};
    task.cacheable = false;

    tachyon::RenderExecutionPlan exec;
    exec.render_plan = plan;
    exec.frame_tasks.push_back(task);
    return exec;
}

} // namespace

bool run_tiling_integration_tests() {
    using namespace tachyon;

    g_failures = 0;

    // --- Visual identity: tiled render == full-frame render ---
    {
        const auto scene = make_solid_scene(64, 64);
        SceneCompiler compiler;
        const auto compiled_result = compiler.compile(scene);
        if (!compiled_result.ok()) {
            std::cerr << "FAIL: scene compilation failed\n";
            return false;
        }
        const CompiledScene& compiled = *compiled_result.value;

        RenderSession full_session;
        const auto full_result = full_session.render(scene, compiled, make_exec_plan(64, 64, 0), "");
        check(!full_result.frames.empty(), "Full-frame render produces a frame");

        RenderSession tiled_session;
        const auto tiled_result = tiled_session.render(scene, compiled, make_exec_plan(64, 64, 16), "");
        check(!tiled_result.frames.empty(), "Tiled render produces a frame");

        if (!full_result.frames.empty() && !tiled_result.frames.empty()) {
            const auto& f = *full_result.frames[0].frame;
            const auto& t = *tiled_result.frames[0].frame;

            check(f.width() == t.width(), "Width matches between tiled and full-frame");
            check(f.height() == t.height(), "Height matches between tiled and full-frame");

            int pixel_mismatches = 0;
            for (uint32_t y = 0; y < f.height() && pixel_mismatches < 5; ++y) {
                for (uint32_t x = 0; x < f.width() && pixel_mismatches < 5; ++x) {
                    const auto p1 = f.get_pixel(x, y);
                    const auto p2 = t.get_pixel(x, y);
                    const float dr = std::abs(p1.r - p2.r);
                    const float dg = std::abs(p1.g - p2.g);
                    const float db = std::abs(p1.b - p2.b);
                    const float da = std::abs(p1.a - p2.a);
                    if (dr > 1e-5f || dg > 1e-5f || db > 1e-5f || da > 1e-5f) {
                        ++pixel_mismatches;
                        std::cerr << "FAIL: pixel mismatch at (" << x << "," << y << ")"
                                  << " full=(" << p1.r << "," << p1.g << "," << p1.b << "," << p1.a << ")"
                                  << " tiled=(" << p2.r << "," << p2.g << "," << p2.b << "," << p2.a << ")\n";
                    }
                }
            }
            if (pixel_mismatches > 0) {
                ++g_failures;
            }
        }
    }

    // --- Border handling: no black border pixels at tile boundaries ---
    {
        const auto scene = make_solid_scene(100, 100);
        SceneCompiler compiler;
        const auto compiled_result = compiler.compile(scene);
        if (!compiled_result.ok()) {
            std::cerr << "FAIL: border scene compilation failed\n";
            ++g_failures;
        } else {
            const CompiledScene& compiled = *compiled_result.value;
            RenderSession session;
            const auto result = session.render(scene, compiled, make_exec_plan(100, 100, 64), "");
            check(!result.frames.empty(), "Border test produces a frame");

            if (!result.frames.empty()) {
                const auto& frame = *result.frames[0].frame;
                // Tile boundary pixel (where 4 tiles meet at x=64, y=64)
                const auto p = frame.get_pixel(64, 64);
                check(p.a > 0.0f, "Tile boundary pixel is not transparent (alpha > 0)");

                // Corner pixel (bottom-right, last tile covers 64..99 x 64..99)
                const auto corner = frame.get_pixel(99, 99);
                check(corner.a > 0.0f, "Corner pixel is not transparent");
            }
        }
    }

    return g_failures == 0;
}
