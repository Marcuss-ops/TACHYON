#include "tachyon/ops/media_ops.h"
#include "tachyon/core/render_graph/render_graph.h"
#include <iostream>
#include <memory>
#include <vector>
#include <future>
#include <chrono>
#include <filesystem>

using namespace tachyon::core;
using namespace tachyon::ops;

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: tachyon-media-test <input-video> <output-dir>\n";
        return 1;
    }

    std::filesystem::path video_path = argv[1];
    std::filesystem::path output_dir = argv[2];

    if (!std::filesystem::exists(video_path)) {
        std::cerr << "Error: Input video does not exist: " << video_path << "\n";
        return 1;
    }

    if (!std::filesystem::exists(output_dir)) {
        std::filesystem::create_directories(output_dir);
    }

    std::cout << "=== Tachyon Media Test Tool (via Operations) ===" << std::endl;
    std::cout << "Input: " << video_path << std::endl;
    std::cout << "Output Dir: " << output_dir << std::endl;
    
    // 3. Run Pipeline in Parallel (10 Instances)
    const int NUM_JOBS = 10;
    std::cout << "[Test] Avvio stress test parallelo (sistema nervoso centrale): " << NUM_JOBS << " istanze..." << std::endl;
    
    std::vector<std::future<RenderResult>> futures;
    auto start_time_total = std::chrono::steady_clock::now();

    for (int i = 0; i < NUM_JOBS; ++i) {
        auto plan_p = std::make_shared<MediaTimelinePlan>("job_" + std::to_string(i));
        plan_p->output.path = (output_dir / ("output_parallel_" + std::to_string(i) + ".mp4")).string();
        plan_p->output.sample_rate = 48000;
        
        VideoTrack vtrack("primary");
        vtrack.is_primary = true;
        vtrack.add_segment(VideoSegment(video_path.string(), 0, 0, 0));
        plan_p->add_track(std::move(vtrack));
        
        RenderGraph g("graph_" + std::to_string(i), plan_p);
        g.config.mode = RenderMode::Fast; 
        
        // Usiamo le OPERATIONS (le estremità) che chiamano il CORE (il cervello)
        futures.push_back(MediaOps::run_pipeline_async(g));
        std::cout << " -> Lanciato Job #" << i << " (via Ops)" << std::endl;
    }
    
    // 4. Wait for all and report
    int success_count = 0;
    for (int i = 0; i < NUM_JOBS; ++i) {
        RenderResult res = futures[i].get();
        if (res.success) {
            success_count++;
            std::cout << "[Job #" << i << "] SUCCESS - Tempo: " << res.drift << "s" << std::endl;
        } else {
            std::cerr << "[Job #" << i << "] FAILURE: " << res.error_message << std::endl;
        }
    }
    
    auto end_time_total = std::chrono::steady_clock::now();
    double total_duration = std::chrono::duration<double>(end_time_total - start_time_total).count();
    
    std::cout << "=== Stress Test Completato ===" << std::endl;
    std::cout << "Successi: " << success_count << " / " << NUM_JOBS << std::endl;
    std::cout << "Tempo totale (muro): " << total_duration << "s" << std::endl;
    
    return 0;
}
