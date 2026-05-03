#include "tachyon/runtime/profiling/render_profiler.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <cassert>
#include <filesystem>

namespace tachyon::profiling {

bool run_profiler_tests() {
    std::cout << "[Profiler] Starting tests...\n";

    RenderProfiler profiler(true);

    {
        ProfileScope scope(&profiler, ProfileEventType::Phase, "test_phase");
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    {
        ProfileScope scope(&profiler, ProfileEventType::Frame, "test_frame", 123);
        {
            ProfileScope layer_scope(&profiler, ProfileEventType::Layer, "test_layer", 123, "layer_1");
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    }

    auto summary = profiler.summarize();
    
    if (summary.total_ms < 15.0) {
        std::cerr << "FAIL: Total time too low: " << summary.total_ms << "ms\n";
        return false;
    }

    if (summary.frames.empty() || summary.frames[0].frame_number != 123) {
        std::cerr << "FAIL: Frame 123 not found in summary\n";
        return false;
    }

    if (summary.layers.empty() || summary.layers[0].layer_id != "layer_1") {
        std::cerr << "FAIL: Layer 'layer_1' not found in summary\n";
        return false;
    }

    std::filesystem::path trace_path = std::filesystem::temp_directory_path() / "tachyon_trace.json";
    if (!profiler.write_chrome_trace_json(trace_path)) {
        std::cerr << "FAIL: Could not write trace JSON to " << trace_path << "\n";
        return false;
    }

    std::cout << "[OK] Profiler tests passed. Trace written to " << trace_path << "\n";
    return true;
}

} // namespace tachyon::profiling
