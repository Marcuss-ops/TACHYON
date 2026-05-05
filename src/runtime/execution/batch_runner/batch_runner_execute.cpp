#include "tachyon/runtime/execution/batch/batch_runner.h"
#include "tachyon/core/spec/compilation/scene_compiler.h"
#include "tachyon/core/spec/compilation/preset_compiler.h"
#include "tachyon/runtime/execution/planning/render_plan.h"
#include "tachyon/runtime/execution/session/render_session.h"
#include "tachyon/runtime/telemetry/render_telemetry_record.h"
#include "tachyon/media/resolution/asset_resolution.h"
#include <atomic>
#include <future>
#include <algorithm>

#include "tachyon/runtime/telemetry/telemetry_writer.h"
#include <chrono>

#include <condition_variable>

namespace tachyon {

namespace {
std::string summarize_diagnostics(const DiagnosticBag& diagnostics, const std::string& prefix) {
    if (diagnostics.diagnostics.empty()) return prefix;
    std::string result = prefix + ": ";
    for (const auto& d : diagnostics.diagnostics) {
        if (d.severity == DiagnosticSeverity::Error) {
            result += d.message + " ";
        }
    }
    return result;
}

std::string get_machine_id() {
    return "local-dev";
}

struct TimeoutGuard {
    CancelFlag& flag;
    std::thread thread;
    std::atomic<bool> done{false};
    std::mutex mutex;
    std::condition_variable cv;

    TimeoutGuard(CancelFlag& f, double seconds) : flag(f) {
        if (seconds > 0.0) {
            thread = std::thread([this, seconds]() {
                std::unique_lock<std::mutex> lock(mutex);
                if (!cv.wait_for(lock, std::chrono::duration<double>(seconds), [this] { return done.load(); })) {
                    flag = true;
                }
            });
        }
    }
    ~TimeoutGuard() {
        {
            std::lock_guard<std::mutex> lock(mutex);
            done = true;
        }
        cv.notify_one();
        if (thread.joinable()) thread.join();
    }
};

}

bool load_scene_context(const std::filesystem::path& /*scene_path*/, SceneSpec& /*scene*/, AssetResolutionTable& /*assets*/, DiagnosticBag& diagnostics) {
    diagnostics.add_error("batch.scene_loading_disabled", "JSON scene loading is no longer supported. Use C++ SceneSpec authoring.");
    return false;
}

bool load_render_job(const std::filesystem::path& /*job_path*/, RenderJob& /*job*/, DiagnosticBag& diagnostics) {
    diagnostics.add_error("batch.job_loading_disabled", "JSON render job loading is no longer supported. Use C++ RenderJob builders.");
    return false;
}

ResolutionResult<RenderBatchResult> run_render_batch(const RenderBatchSpec& spec, std::size_t worker_count) {
    ResolutionResult<RenderBatchResult> result;
    const std::size_t concurrency = std::max<std::size_t>(1, worker_count == 0 ? spec.worker_count : worker_count);
    RenderBatchResult batch_result;
    batch_result.jobs.resize(spec.jobs.size());

    const std::string run_id = spec.batch_id.empty() ? "batch-" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count()) : spec.batch_id;
    const std::string machine_id = get_machine_id();

    std::atomic<std::size_t> next_index{0};
    std::vector<std::future<void>> workers;
    workers.reserve(std::min(concurrency, spec.jobs.size()));

    for (std::size_t worker = 0; worker < std::min(concurrency, spec.jobs.size()); ++worker) {
        workers.push_back(std::async(std::launch::async, [&]() {
            for (;;) {
                const std::size_t index = next_index.fetch_add(1);
                if (index >= spec.jobs.size()) return;

                const RenderBatchItem& request = spec.jobs[index];
                RenderBatchJobResult job_result;
                job_result.request = request;

                int attempts = 0;
                const int max_attempts = std::max(1, request.retry_policy.max_attempts);
                const double timeout = request.timeout_seconds.value_or(spec.default_timeout_seconds.value_or(0.0));

                while (attempts < max_attempts) {
                    attempts++;
                    
                    DiagnosticBag diagnostics;
                    SceneSpec scene;
                    AssetResolutionTable assets;
                    if (!load_scene_context(request.scene_path, scene, assets, diagnostics)) {
                        job_result.error = summarize_diagnostics(diagnostics, "failed to load scene");
                        break; // Scene loading error is usually not retryable
                    }

                    // Expand preset animations into concrete keyframes
                    {
                        PresetLibrary preset_library;
                        const std::filesystem::path preset_dir = request.scene_path.parent_path() / "presets";
                        if (preset_library.load_from_directory(preset_dir)) {
                            PresetCompiler compiler(preset_library);
                            compiler.expand(scene);
                        }
                    }

                    RenderJob job;
                    if (!load_render_job(request.job_path, job, diagnostics)) {
                        job_result.error = summarize_diagnostics(diagnostics, "failed to load job");
                        break; // Job loading error is usually not retryable
                    }

                    if (request.output_override.has_value()) job.output.destination.path = request.output_override->string();

                    const auto plan_result = build_render_plan(scene, job);
                    if (!plan_result.value.has_value()) { job_result.error = "failed to build render plan"; break; }

                    const auto execution_result = build_render_execution_plan(*plan_result.value, assets.size());
                    if (!execution_result.value.has_value()) { job_result.error = "failed to build execution plan"; break; }

                    SceneCompiler compiler;
                    const auto compiled_result = compiler.compile(scene);
                    if (!compiled_result.ok()) { job_result.error = "failed to compile scene"; break; }

                    RenderSession session;
                    const std::filesystem::path output_path = job.output.destination.path.empty() ? std::filesystem::path{} : std::filesystem::path(job.output.destination.path);
                    
                    CancelFlag cancel_flag{false};
                    {
                        TimeoutGuard timeout_guard(cancel_flag, timeout);
                        job_result.session_result = session.render(scene, *compiled_result.value, *execution_result.value, output_path, concurrency, &cancel_flag);
                    }
                    
                    job_result.success = job_result.session_result.output_error.empty() && !cancel_flag;
                    
                    if (cancel_flag) {
                        job_result.error = "timeout after " + std::to_string(timeout) + "s";
                    } else if (!job_result.success) {
                        job_result.error = job_result.session_result.output_error;
                    }

                    // Telemetry
                    RenderTelemetryContext context;
                    context.run_id = run_id;
                    context.machine_id = machine_id;
                    context.scene_id = request.scene_path.stem().string();
                    context.frames_total = static_cast<int>(job.frame_range.end - job.frame_range.start);
                    
                    job_result.telemetry = make_render_telemetry_record(job, job_result.session_result, context);
                    if (cancel_flag) {
                        job_result.telemetry.error_code = to_string(RenderErrorCode::Timeout);
                        job_result.telemetry.success = false;
                    }

                    if (job_result.success) break;
                    
                    if (attempts < max_attempts) {
                        std::this_thread::sleep_for(std::chrono::duration<double>(request.retry_policy.backoff_seconds));
                    }
                }

                // Persistence
                const auto telemetry_dir = TelemetryWriter::get_default_directory();
                TelemetryWriter::append_jsonl(job_result.telemetry, telemetry_dir / (run_id + ".jsonl"));

                batch_result.jobs[index] = std::move(job_result);
            }
        }));
    }

    for (auto& worker : workers) {
        try { worker.get(); }
        catch (const std::exception& error) { result.diagnostics.add_error("batch.execution_failed", error.what()); }
    }

    std::vector<RenderTelemetryRecord> all_telemetry;
    all_telemetry.reserve(batch_result.jobs.size());

    for (const auto& job : batch_result.jobs) {
        all_telemetry.push_back(job.telemetry);
        if (job.success) ++batch_result.succeeded;
        else {
            ++batch_result.failed;
            if (!job.error.empty()) result.diagnostics.add_error("batch.job_failed", job.error);
        }
    }

    batch_result.summary = aggregate_telemetry(all_telemetry);
    batch_result.summary.batch_id = run_id;

    const auto telemetry_dir = TelemetryWriter::get_default_directory();
    TelemetryWriter::write_batch_summary(batch_result.summary, telemetry_dir / (run_id + "_summary.json"));

    result.value = std::move(batch_result);
    return result;
}

} // namespace tachyon
