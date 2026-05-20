#include "test_utils.h"
#include "tachyon/runtime/execution/session/render_session.h"
#include "tachyon/runtime/execution/session/render_internal.h"
#include "tachyon/runtime/compiler/scene_compiler.h"
#include "tachyon/runtime/execution/planning/render_plan.h"
#include "tachyon/runtime/execution/jobs/render_job.h"
#include "tachyon/scene/builder.h"
#include "tachyon/output/frame_output_sink.h"
#include "tachyon/runtime/resource/surface_pool.h"
#include <iostream>
#include <vector>
#include <mutex>
#include <algorithm>

using namespace tachyon;

class MockSink : public output::FrameOutputSink {
public:
    bool begin(const RenderPlan& plan) override { (void)plan; return true; }
    bool write_frame(const output::OutputFramePacket& packet) override {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_frame_numbers.push_back(packet.frame_number);
        return true;
    }
    bool finish() override { return true; }
    
    [[nodiscard]] const std::string& last_error() const override { return m_error; }
    
    std::vector<std::int64_t> frame_numbers() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_frame_numbers;
    }

private:
    std::vector<std::int64_t> m_frame_numbers;
    mutable std::mutex m_mutex;
    std::string m_error;
};

bool run_render_streaming_tests() {
    std::cout << "[RenderStreamingTests] Starting RenderStreaming unit tests...\n";

    auto scene_spec = scene::Composition("streaming_test")
        .size(64, 64)
        .duration(1.0)
        .fps(30)
        .layer("bg", [](scene::LayerBuilder& l) {
            l.solid("bg").color({255, 0, 0, 255});
        })
        .build_scene();

    SceneCompiler compiler;
    auto compiled_res = compiler.compile(scene_spec);
    if (!compiled_res.diagnostics.ok() || !compiled_res.value.has_value()) return false;
    const auto& compiled = *compiled_res.value;

    RenderJob job = RenderJobBuilder::video_export("streaming_test", {0, 9}, "dummy_output");
    auto plan_res = build_render_plan(scene_spec, job);
    if (!plan_res.diagnostics.ok() || !plan_res.value.has_value()) return false;
    
    auto exec_res = build_render_execution_plan(*plan_res.value, 0);
    if (!exec_res.diagnostics.ok() || !exec_res.value.has_value()) return false;
    
    const auto& execution_plan = *exec_res.value;
    
    FrameCache cache;
    runtime::RenderWorkerBudget budget;
    budget.frame_concurrency = 4;
    budget.pixel_concurrency = 1;

    RenderContext context;
    context.surface_pool = std::make_shared<SurfacePool>();
    context.surface_pool->prepare(64, 64, 4);

    // Test 1: Streaming mode writes frames in order
    {
        MockSink sink;
        std::vector<ExecutedFrame> rendered_frames;
        RenderSessionResult result;

        render_frames_parallel_internal(
            compiled,
            execution_plan,
            cache,
            budget,
            context,
            nullptr, // prefetcher
            nullptr, // scheduler
            rendered_frames,
            nullptr, // progress
            nullptr, // cancel
            nullptr, // disk_cache
            &sink,
            &result
        );

        auto frame_nums = sink.frame_numbers();
        if (frame_nums.size() != 10) {
            std::cerr << "[RenderStreamingTests] FAIL: Expected 10 frames written, got " << frame_nums.size() << "\n";
            return false;
        }

        for (std::int64_t i = 0; i < 10; ++i) {
            if (frame_nums[i] != i) {
                std::cerr << "[RenderStreamingTests] FAIL: Frame written out of order at index " << i << ". Got " << frame_nums[i] << "\n";
                return false;
            }
        }

        // Check shell frames
        for (const auto& f : rendered_frames) {
            if (f.frame != nullptr) {
                std::cerr << "[RenderStreamingTests] FAIL: Rendered frame buffer should be nullptr in streaming mode\n";
                return false;
            }
        }

        std::cout << "[RenderStreamingTests] Streaming mode order and shell frames verified!\n";
    }

    std::cout << "[RenderStreamingTests] ALL TESTS PASSED!\n";
    return true;
}
