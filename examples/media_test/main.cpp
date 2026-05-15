#include "tachyon/media/pipeline_orchestrator.h"
#include "tachyon/core/render_graph/render_graph.h"
#include <iostream>
#include <memory>
#include <vector>
#include <future>
#include <chrono>

using namespace tachyon::core;
using namespace tachyon::ops;

int main() {
    std::cout << "=== Tachyon Media Test Tool (via Operations) ===" << std::endl;
    
    std::string video_path = "C:\\Users\\pater\\Downloads\\New Youtube Video .mp4";
    
    // 3. Run Pipeline in Parallel (10 Instances)
    const int NUM_JOBS = 10;
    std::cout << "[Test] Avvio stress test parallelo (sistema nervoso centrale): " << NUM_JOBS << " istanze..." << std::endl;
    
    std::vector<std::future<RenderResult>> futures;
    auto start_time_total = std::chrono::steady_clock::now();

    for (int i = 0; i < NUM_JOBS; ++i) {
        auto plan_p = std::make_shared<MediaTimelinePlan>("job_" + std::to_string(i));
        plan_p->output.path = "C:\\Users\\pater\\Downloads\\output_parallel_" + std::to_string(i) + ".mp4";
        plan_p->output.sample_rate = 48000;
        
        VideoTrack vtrack("primary");
        vtrack.is_primary = true;
        vtrack.add_segment(VideoSegment(video_path, 0, 0, 0));
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
