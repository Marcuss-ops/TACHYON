#include "tachyon/core/cli.h"
#include "tachyon/core/cli_options.h"
#include "tachyon/core/report.h"
#include "tachyon/core/cli_scene_loader.h"
#include "tachyon/scene/builder.h"
#include "tachyon/runtime/execution/native_render.h"
#include "cli_internal.h"
#include <iomanip>
#include <iostream>
#include <thread>
#include <string_view>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <cmath>
#include <numeric>
#include <sstream>

namespace tachyon {

namespace {
struct TimingSummary {
    std::string category;
    std::string label;
    double total_ms{0.0};
    double max_ms{0.0};
    std::size_t samples{0};
};

std::vector<TimingSummary> aggregate_timings(const std::vector<FrameDiagnostics>& frames) {
    std::unordered_map<std::string, TimingSummary> buckets;
    for (const auto& frame : frames) {
        for (const auto& timing : frame.timings) {
            const std::string key = timing.category + "\n" + timing.label;
            auto& bucket = buckets[key];
            if (bucket.samples == 0) {
                bucket.category = timing.category;
                bucket.label = timing.label;
            }
            bucket.total_ms += timing.milliseconds;
            bucket.max_ms = std::max(bucket.max_ms, timing.milliseconds);
            bucket.samples++;
        }
    }

    std::vector<TimingSummary> summaries;
    summaries.reserve(buckets.size());
    for (auto& [_, bucket] : buckets) {
        summaries.push_back(std::move(bucket));
    }
    std::sort(summaries.begin(), summaries.end(), [](const auto& a, const auto& b) {
        if (a.total_ms != b.total_ms) return a.total_ms > b.total_ms;
        return a.samples > b.samples;
    });
    return summaries;
}

double percentile_from_sorted(const std::vector<double>& sorted, double p) {
    if (sorted.empty()) {
        return 0.0;
    }
    const double clamped = std::clamp(p, 0.0, 1.0);
    const double pos = clamped * static_cast<double>(sorted.size() - 1);
    const std::size_t lo = static_cast<std::size_t>(std::floor(pos));
    const std::size_t hi = static_cast<std::size_t>(std::ceil(pos));
    if (lo == hi) {
        return sorted[lo];
    }
    const double t = pos - static_cast<double>(lo);
    return sorted[lo] * (1.0 - t) + sorted[hi] * t;
}

void print_timing_hotspots(const RenderSessionResult& session_result, std::ostream& out) {
    const auto hotspots = aggregate_timings(session_result.frame_diagnostics);
    if (hotspots.empty()) {
        return;
    }

    out << "\n  render hotspots\n";
    out << "  ─────────────────────────────────────────\n";
    out << "  bucket                      total      avg     peak   samples\n";
    out << "  ───────────────────────────────────────────────────────────────\n";

    const std::size_t limit = std::min<std::size_t>(8, hotspots.size());
    for (std::size_t i = 0; i < limit; ++i) {
        const auto& bucket = hotspots[i];
        const double avg = bucket.samples > 0 ? bucket.total_ms / static_cast<double>(bucket.samples) : 0.0;
        std::ostringstream line;
        line << bucket.category;
        if (!bucket.label.empty()) {
            line << ":" << bucket.label;
        }
        const std::string name = line.str();
        out << "  " << std::left << std::setw(27) << name.substr(0, 27)
            << std::right << std::setw(8) << std::fixed << std::setprecision(1) << bucket.total_ms << "ms"
            << std::setw(8) << avg << "ms"
            << std::setw(8) << bucket.max_ms << "ms"
            << std::setw(8) << bucket.samples << '\n';
    }
}

void print_speed_hints(const RenderExecutionPlan& execution_plan, const RenderSessionResult& session_result, std::ostream& out) {
    const std::size_t frame_count = session_result.frame_times_ms.size();
    if (frame_count == 0) {
        return;
    }

    std::vector<double> sorted = session_result.frame_times_ms;
    std::sort(sorted.begin(), sorted.end());
    const double avg = std::accumulate(sorted.begin(), sorted.end(), 0.0) / static_cast<double>(sorted.size());
    const double p95 = percentile_from_sorted(sorted, 0.95);
    const double p50 = percentile_from_sorted(sorted, 0.50);

    out << "\n  speed hints\n";
    out << "  ─────────────────────────────────────────\n";
    out << "  frame median " << std::fixed << std::setprecision(1) << p50
        << "ms  p95 " << p95 << "ms  avg " << avg << "ms\n";

    const double cache_hit_rate = session_result.cache_hit_rate();
    if (cache_hit_rate < 35.0) {
        out << "  cache pressure: " << static_cast<int>(cache_hit_rate)
            << "% hit rate. Precomp more, reuse assets, and avoid over-animating properties.\n";
    }

    const auto hotspots = aggregate_timings(session_result.frame_diagnostics);
    if (!hotspots.empty()) {
        const auto& slowest = hotspots.front();
        const double slowest_avg = slowest.samples > 0 ? slowest.total_ms / static_cast<double>(slowest.samples) : 0.0;
        if (slowest.category == "effect") {
            out << "  slowest effect: " << slowest.label << " (~" << std::fixed << std::setprecision(1) << slowest_avg
                << "ms per call). Trim blur radius, glow strength, or chain length.\n";
        } else if (slowest.category == "layer_surface") {
            out << "  slowest layer: " << slowest.label << " (~" << std::fixed << std::setprecision(1) << slowest_avg
                << "ms per render). Flatten precomps, cut source resolution, or simplify masks/text.\n";
        } else if (slowest.category == "adjustment") {
            out << "  slowest adjustment layer: " << slowest.label << " (~" << std::fixed << std::setprecision(1) << slowest_avg
                << "ms). Move heavy effects earlier or reduce full-frame passes.\n";
        }
    }

    if (execution_plan.render_plan.composition.layer_count > 0 && session_result.frame_execution_ms > 0.0) {
        if (session_result.encode_ms < session_result.frame_execution_ms * 0.25) {
            out << "  render time is dominated by frame execution, not encode. Focus on layer counts, effects, and cache hits first.\n";
        }
    }
}

void print_execution_plan(
    const RenderExecutionPlan& execution_plan,
    const RasterizedFrame2D& rasterized_first_frame,
    const RenderSessionResult& session_result,
    std::ostream& out,
    std::size_t worker_count = 1) {
    out << "render execution plan valid\n";
    out << "composition: " << execution_plan.render_plan.composition_target << " (" << execution_plan.render_plan.composition.width << "x"
        << execution_plan.render_plan.composition.height << " @ " << execution_plan.render_plan.composition.frame_rate.value() << " fps, "
        << execution_plan.render_plan.composition.layer_count << " layers)\n";
    out << "frames: " << execution_plan.render_plan.frame_range.start << " -> " << execution_plan.render_plan.frame_range.end << '\n';

    // Phase timing table
    const double total_phase_time = session_result.scene_compile_ms + session_result.plan_build_ms 
                                 + session_result.execution_plan_build_ms + session_result.frame_execution_ms 
                                 + session_result.encode_ms;
    
    out << "\n  phase                      time        %\n";
    out << "  ─────────────────────────────────────────\n";
    constexpr std::size_t kPhaseColumnWidth = 27;
    constexpr std::size_t kTimeColumnWidth = 8;

    auto print_phase = [&](std::string_view name, double ms) {
        const double pct = total_phase_time > 0 ? (ms / total_phase_time) * 100.0 : 0.0;
        const std::size_t pad = name.size() < kPhaseColumnWidth ? (kPhaseColumnWidth - name.size()) : 1;
        out << "  " << name << std::string(pad, ' ')
            << std::fixed << std::setprecision(1) << std::setw(kTimeColumnWidth) << ms << "ms  "
            << std::fixed << std::setprecision(0) << std::setw(3) << pct << "%\n";
    };

    print_phase("scene compile",        session_result.scene_compile_ms);
    print_phase("plan build",           session_result.plan_build_ms);
    print_phase("execution plan build", session_result.execution_plan_build_ms);
    print_phase("frame execution",      session_result.frame_execution_ms);

    std::string encode_phase = "encode";
    if (!execution_plan.render_plan.output.profile.video.codec.empty())
        encode_phase += " (" + execution_plan.render_plan.output.profile.video.codec + ")";
    print_phase(encode_phase, session_result.encode_ms);

    out << "  ─────────────────────────────────────────\n";
    print_phase("total", total_phase_time);
    out << "\n";

    // Frame stats
    if (!session_result.frame_times_ms.empty()) {
        double sum = 0.0;
        double peak = 0.0;
        std::size_t peak_frame = 0;
        for (std::size_t i = 0; i < session_result.frame_times_ms.size(); ++i) {
            const double t = session_result.frame_times_ms[i];
            sum += t;
            if (t > peak) {
                peak = t;
                peak_frame = i;
            }
        }
        const double avg = sum / session_result.frame_times_ms.size();
        out << "  frames: " << session_result.frames_written << "  avg " << avg << "ms/frame  peak " 
            << peak << "ms (frame " << peak_frame << ")\n";
    }

    // Cache stats
    out << "  cache: " << session_result.cache_hits << " hits / " << session_result.cache_misses << " misses ("
        << static_cast<int>(session_result.cache_hit_rate()) << "%)\n";

    // Memory and workers
    const std::size_t peak_memory_bytes = std::max(session_result.peak_working_set_bytes, session_result.peak_private_bytes);
    if (peak_memory_bytes > 0) {
        out << "  memory: peak " << (peak_memory_bytes / (1024 * 1024)) << "MB";
        if (session_result.peak_working_set_bytes > 0 && session_result.peak_private_bytes > 0) {
            out << " (working set " << (session_result.peak_working_set_bytes / (1024 * 1024))
                << "MB, private " << (session_result.peak_private_bytes / (1024 * 1024)) << "MB)";
        }
        out << '\n';
    }
    out << "  workers: " << worker_count << " threads\n";
    out << "  2d runtime backend: " << rasterized_first_frame.backend_name << '\n';
    print_timing_hotspots(session_result, out);
    print_speed_hints(execution_plan, session_result, out);
}

}

bool run_render_command(const CliOptions& options, std::ostream& out, std::ostream& err, TransitionRegistry& transition_registry) {
    SceneLoadOptions load_opts;
    load_opts.cpp_path = options.cpp_path;
    load_opts.preset_id = options.preset_id;

    auto loaded = load_scene_for_cli(load_opts, SceneLoadMode::Render, out, err);
    if (!loaded.success) {
        print_diagnostics(loaded.diagnostics, err);
        return false;
    }

    SceneSpec& scene = loaded.context->scene;
    AssetResolutionTable& assets = loaded.context->assets;

    RenderJob job;
    {
        // Default job logic
        if (scene.compositions.empty()) {
            err << "Scene has no compositions.\n";
            return false;
        }
        const auto& comp = scene.compositions.front();
        FrameRange range = {0, static_cast<std::int64_t>(comp.duration * comp.frame_rate.value())};
        std::string output = !options.output_override.empty() ? options.output_override.string() : "output.mp4";
        job = RenderJobBuilder::video_export(comp.id, range, output);
    }

    if (!options.output_override.empty()) job.output.destination.path = options.output_override.string();
    if (options.output_preset_id.has_value()) job.output.profile.name = *options.output_preset_id;
    if (options.frame_range_override.has_value()) job.frame_range = *options.frame_range_override;

    NativeRenderOptions native_options;
    native_options.worker_count = options.worker_count;
    native_options.memory_budget_bytes = options.memory_budget_bytes;
    native_options.verbose = true;

    const RenderSessionResult session_result = NativeRenderer::render(scene, job, transition_registry, native_options);
    
    if (!session_result.output_error.empty()) {
        err << "Render error: " << session_result.output_error << "\n";
    }

    print_diagnostics(session_result.diagnostics, err);

    {
        RasterizedFrame2D first_frame;
        if (!session_result.frames.empty()) {
            first_frame.backend_name = "cpu-frame-executor";
        }
        
        // We need a RenderExecutionPlan for the execution plan printout
        const auto plan_result = build_render_plan(scene, job);
        if (plan_result.value.has_value()) {
            const auto execution_result = build_render_execution_plan(*plan_result.value, assets.size());
            if (execution_result.value.has_value()) {
                const std::size_t workers = (options.worker_count > 0) ? options.worker_count : std::thread::hardware_concurrency();
                print_execution_plan(*execution_result.value, first_frame, session_result, out, workers);
            }
        }
    }
    return session_result.output_error.empty();
}

bool run_preview_command(const CliOptions& options, std::ostream& out, std::ostream& err, TransitionRegistry& registry) {
    return run_preview_internal(options, out, err, "NativePreview", registry);
}

} // namespace tachyon
