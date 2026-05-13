#include "tachyon/runtime/execution/session/render_session.h"
#include "tachyon/core/spec/schema/objects/scene_spec.h"
#include "tachyon/scene/builder.h"
#include "tachyon/runtime/compiler/scene_compiler.h"
#include "tachyon/runtime/execution/planning/render_plan.h"
#include "tachyon/runtime/execution/parallel/taskflow_runtime.h"
#include "tachyon/core/profiling.h"
#include "tachyon/renderer2d/effects/effect_registry.h"
#include "tachyon/transition_registry.h"
#include "tachyon/presets/text/text_registry.h"
#include "tachyon/runtime/registry/runtime_registry_bundle.h"
#include "tachyon/runtime/policy/worker_budget.h"
#include "tachyon/runtime/core/data/compiled_scene.h"

#include <iostream>
#include <chrono>
#include <vector>
#include <numeric>
#include <iomanip>

using namespace tachyon;

namespace {

SceneSpec create_composite_bench_scene(int layer_count, int width, int height) {
    auto scene_builder = scene::Scene();
    
    scene_builder.composition("main", [&](scene::CompositionBuilder& comp) {
        comp.size(width, height)
            .duration(1.0)
            .fps(30);

        for (int i = 0; i < layer_count; ++i) {
            std::string layer_id = "layer_" + std::to_string(i);
            comp.layer(layer_id, [&](scene::LayerBuilder& lb) {
                lb.solid(layer_id)
                  .color({255, 0, 0, 255}) // Red
                  .opacity(0.5); // Alpha blending is expensive, good for bench
            });
        }
    });

    return scene_builder.build();
}

} // namespace

int main(int argc, char** argv) {
    int layer_count = 100;
    int width = 1920;
    int height = 1080;
    int iterations = 10;

    if (argc > 1) layer_count = std::atoi(argv[1]);
    if (argc > 2) iterations = std::atoi(argv[2]);

    std::cout << "--- TACHYON Benchmarking Tool ---" << std::endl;
    std::cout << "Layers: " << layer_count << std::endl;
    std::cout << "Resolution: " << width << "x" << height << std::endl;
    std::cout << "Iterations: " << iterations << std::endl;
    std::cout << "---------------------------------" << std::endl;

    SceneSpec scene = create_composite_bench_scene(layer_count, width, height);
    
    // 1. Compile Scene
    SceneCompiler compiler;
    auto compile_result = compiler.compile(scene);
    if (!compile_result.ok()) {
        std::cerr << "Compilation failed!" << std::endl;
        return 1;
    }
    const auto& compiled = compile_result.value.value();

    // 2. Setup Render Session
    RenderSession session;
    
    // 3. Setup Render Plan (Render frame 0 of "main")
    RenderExecutionPlan execution_plan;
    execution_plan.render_plan.composition_target = "main";
    execution_plan.render_plan.composition.width = width;
    execution_plan.render_plan.composition.height = height;
    execution_plan.render_plan.composition.frame_rate = {30, 1};
    
    FrameRenderTask task;
    task.frame_number = 0;
    task.time_seconds = 0.0;
    execution_plan.frame_tasks.push_back(task);

    // 4. Budget
    runtime::RenderWorkerBudget budget;
    budget.total_threads = 0; // Use all cores
    
    // Warmup
    std::cout << "Warming up..." << std::endl;
    for (int i = 0; i < 3; ++i) {
        session.render(scene, compiled, execution_plan, "", budget);
    }

    std::vector<double> timings;
    timings.reserve(iterations);

    for (int i = 0; i < iterations; ++i) {
        auto start = std::chrono::high_resolution_clock::now();
        
        // Trigger render
        auto result = session.render(scene, compiled, execution_plan, "", budget);
        if (result.frames.empty()) {
            std::cerr << "Render failed!" << std::endl;
            return 1;
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        double ms = std::chrono::duration<double, std::milli>(end - start).count();
        timings.push_back(ms);
        
        std::cout << "Iteration " << i << ": " << std::fixed << std::setprecision(2) << ms << " ms" << std::endl;
    }

    double avg = std::accumulate(timings.begin(), timings.end(), 0.0) / iterations;
    std::cout << "---------------------------------" << std::endl;
    std::cout << "Average: " << std::fixed << std::setprecision(2) << avg << " ms/frame" << std::endl;

    return 0;
}
