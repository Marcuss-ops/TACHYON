#pragma once
#include "tachyon/core/platform/pipe_process.h"
#include "tachyon/renderer2d/evaluated_composition/composition_renderer.h"
#include "tachyon/core/scene/evaluation/evaluator.h"
#include "tachyon/render/render_intent.h"
#include "tachyon/renderer2d/effects/effect_registry.h"
#include "tachyon/transition_registry.h"
#include "tachyon/core/spec/schema/objects/scene_spec.h"
#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <queue>
#include <functional>
#include <atomic>
#include <iomanip>

namespace tachyon::renderer2d {

// --- Simple ThreadPool Implementation ---
class ThreadPool {
public:
    explicit ThreadPool(size_t threads) : stop(false) {
        for(size_t i = 0; i < threads; ++i)
            workers.emplace_back([this] {
                for(;;) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(this->queue_mutex);
                        this->condition.wait(lock, [this]{ return this->stop || !this->tasks.empty(); });
                        if(this->stop && this->tasks.empty()) return;
                        task = std::move(this->tasks.front());
                        this->tasks.pop();
                    }
                    task();
                }
            });
    }

    template<class F>
    void enqueue(F&& f) {
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            tasks.emplace(std::forward<F>(f));
        }
        condition.notify_one();
    }

    void wait_and_stop() {
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            stop = true;
        }
        condition.notify_all();
        for(std::thread &worker: workers) worker.join();
    }

    ~ThreadPool() {
        if (!stop) wait_and_stop();
    }

private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    std::mutex queue_mutex;
    std::condition_variable condition;
    bool stop;
};

// --- Robust Rendering Utility ---

struct RobustRenderConfig {
    static constexpr int DefaultWidth = 1920;
    static constexpr int DefaultHeight = 1080;
    static constexpr int DefaultFPS = 60; // Upgraded to 60fps as new standard
    static constexpr double DefaultDuration = 2.0;

    int width = DefaultWidth;
    int height = DefaultHeight;
    int fps = DefaultFPS;
    double duration = DefaultDuration;
    int crf = 18;
    std::string preset = "medium";
};

struct RenderMetrics {
    std::atomic<double> total_gen_time_ms{0};
    std::atomic<size_t> bytes_sent{0};
    std::atomic<int> frames_done{0};
    std::chrono::steady_clock::time_point start_time;

    void report(int total_frames, int width, int height) {
        auto now = std::chrono::steady_clock::now();
        double elapsed_sec = std::chrono::duration<double>(now - start_time).count();
        int done = frames_done.load();
        double avg_gen_ms = total_gen_time_ms.load() / (done > 0 ? done : 1);
        double throughput_mbs = (bytes_sent.load() / (1024.0 * 1024.0)) / (elapsed_sec > 0 ? elapsed_sec : 1.0);
        double fps = done / (elapsed_sec > 0 ? elapsed_sec : 1.0);
        
        std::cout << "[RENDER] " << std::setw(3) << (100 * done / total_frames) << "% | "
                  << "Done: " << done << "/" << total_frames << " | "
                  << std::fixed << std::setprecision(1) << fps << " fps | "
                  << "Gen: " << avg_gen_ms << "ms/f | "
                  << throughput_mbs << " MB/s throughput" << std::endl;
    }
};

inline void render_scene_to_mp4(
    const tachyon::SceneSpec& scene,
    const std::string& comp_id,
    const std::string& output_path,
    const TransitionRegistry& trans_reg,
    const RobustRenderConfig& config = {}
) {
    std::cout << "[DEBUG] Entering render_scene_to_mp4 for " << comp_id << std::endl;
    using namespace tachyon::core::platform;

    const int TOTAL_FRAMES = static_cast<int>(config.fps * config.duration);
    const size_t FRAME_SIZE = config.width * config.height * 4;

    std::string ffmpeg_cmd = 
        "ffmpeg -y -loglevel error -f rawvideo -pix_fmt rgba -s " + std::to_string(config.width) + "x" + std::to_string(config.height) + 
        " -framerate " + std::to_string(config.fps) + " -thread_queue_size 1024 -i - "
        "-r " + std::to_string(config.fps) + " -c:v libx264 -preset " + config.preset + " -crf " + std::to_string(config.crf) + 
        " -pix_fmt yuv420p -movflags +faststart -color_primaries bt709 -color_trc bt709 -colorspace bt709 \"" + output_path + "\"";

    FILE* pipe = open_write_pipe(ffmpeg_cmd.c_str());
    if (!pipe) {
        std::cerr << "[ERROR] Failed to open FFmpeg pipe: " << ffmpeg_cmd << "\n";
        return;
    }

    RenderMetrics metrics;
    metrics.start_time = std::chrono::steady_clock::now();

    std::mutex pipe_mutex;
    std::atomic<int> next_to_write{0};
    std::vector<std::vector<uint8_t>> slots(TOTAL_FRAMES);
    std::vector<bool> ready(TOTAL_FRAMES, false);

    auto worker_task = [&](int i) {
        auto gen_start = std::chrono::steady_clock::now();
        
        double t = (double)i / (double)config.fps;
        auto evaluated = tachyon::scene::evaluate_scene_composition_state(scene, comp_id, t);
        
        EffectRegistry thread_reg;
        RenderPlan thread_plan;
        RenderContext ctx;
        ctx.transition_registry = &trans_reg;
        render::RenderIntent intent; // Default
        
        FrameRenderTask task;
        task.time_seconds = t;
        task.frame_number = i;

        auto result = render_evaluated_composition_2d(*evaluated, intent, thread_plan, task, ctx, thread_reg);
        if (result.surface) {
            result.surface->to_rgba8(slots[i]);
        } else {
            slots[i].assign(FRAME_SIZE, 0); // Black frame on error
        }

        auto gen_end = std::chrono::steady_clock::now();
        metrics.total_gen_time_ms = metrics.total_gen_time_ms + std::chrono::duration<double, std::milli>(gen_end - gen_start).count();

        // Ordered Writing Logic
        {
            std::lock_guard<std::mutex> lock(pipe_mutex);
            ready[i] = true;
            
            while (next_to_write < TOTAL_FRAMES && ready[next_to_write]) {
                int idx = next_to_write;
                size_t written = std::fwrite(slots[idx].data(), 1, slots[idx].size(), pipe);
                metrics.bytes_sent += written;
                
                // Clear and shrink to free memory immediately
                slots[idx].clear();
                slots[idx].shrink_to_fit();
                
                next_to_write++;
                metrics.frames_done++;
                
                if (metrics.frames_done % 10 == 0 || metrics.frames_done == TOTAL_FRAMES) {
                    metrics.report(TOTAL_FRAMES, config.width, config.height);
                }
            }
        }
    };

    unsigned int num_workers = std::thread::hardware_concurrency();
    if (num_workers == 0) num_workers = 4;
    
    std::cout << "[RENDER] Starting Robust Pipeline (" << num_workers << " workers, " << config.width << "x" << config.height << ")\n";

    ThreadPool pool(num_workers);
    for (int i = 0; i < TOTAL_FRAMES; ++i) {
        pool.enqueue([&worker_task, i] { worker_task(i); });
    }

    pool.wait_and_stop();
    
    int ret = close_pipe(pipe);
    std::cout << "\n[RENDER] Complete. FFmpeg exit code: " << ret << "\n";
    std::cout << "[RENDER] Output: " << output_path << "\n";
}

} // namespace tachyon::renderer2d
