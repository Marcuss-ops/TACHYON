#include "batch_runner_internal.h"
#include "tachyon/core/spec/compilation/scene_compiler.h"
#include "tachyon/core/spec/compilation/preset_compiler.h"
#include "tachyon/runtime/execution/planning/render_plan.h"
#include "tachyon/runtime/execution/session/render_session.h"
#include "tachyon/media/resolution/asset_resolution.h"
#include <atomic>
#include <future>
#include <algorithm>

namespace tachyon {

bool load_scene_context(const std::filesystem::path& scene_path, SceneSpec& scene, AssetResolutionTable& assets, DiagnosticBag& diagnostics) {
    const auto parsed = parse_scene_spec_file(scene_path);
    if (!parsed.value.has_value()) { diagnostics.append(parsed.diagnostics); return false; }
    const auto validation = validate_scene_spec(*parsed.value);
    if (!validation.ok()) { diagnostics.append(validation.diagnostics); return false; }
    const auto resolved_assets = resolve_assets(*parsed.value, scene_asset_root(scene_path));
    if (!resolved_assets.value.has_value()) { diagnostics.append(resolved_assets.diagnostics); return false; }
    scene = *parsed.value; assets = *resolved_assets.value;
    return true;
}

bool load_render_job(const std::filesystem::path& job_path, RenderJob& job, DiagnosticBag& diagnostics) {
    const auto parsed = parse_render_job_file(job_path);
    if (!parsed.value.has_value()) { diagnostics.append(parsed.diagnostics); return false; }
    const auto validation = validate_render_job(*parsed.value);
    if (!validation.ok()) { diagnostics.append(validation.diagnostics); return false; }
    job = *parsed.value;
    return true;
}

ResolutionResult<RenderBatchResult> run_render_batch(const RenderBatchSpec& spec, std::size_t worker_count) {
    ResolutionResult<RenderBatchResult> result;
    const std::size_t concurrency = std::max<std::size_t>(1, worker_count == 0 ? spec.worker_count : worker_count);
    RenderBatchResult batch_result;
    batch_result.jobs.resize(spec.jobs.size());

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

                DiagnosticBag diagnostics;
                SceneSpec scene;
                AssetResolutionTable assets;
                if (!load_scene_context(request.scene_path, scene, assets, diagnostics)) {
                    job_result.error = summarize_diagnostics(diagnostics, "failed to load scene");
                    batch_result.jobs[index] = std::move(job_result); continue;
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
                    batch_result.jobs[index] = std::move(job_result); continue;
                }

                if (request.output_override.has_value()) job.output.destination.path = request.output_override->string();

                const auto plan_result = build_render_plan(scene, job);
                if (!plan_result.value.has_value()) { job_result.error = "failed to build render plan"; batch_result.jobs[index] = std::move(job_result); continue; }

                const auto execution_result = build_render_execution_plan(*plan_result.value, assets.size());
                if (!execution_result.value.has_value()) { job_result.error = "failed to build execution plan"; batch_result.jobs[index] = std::move(job_result); continue; }

                SceneCompiler compiler;
                const auto compiled_result = compiler.compile(scene);
                if (!compiled_result.ok()) { job_result.error = "failed to compile scene"; batch_result.jobs[index] = std::move(job_result); continue; }

                RenderSession session;
                const std::filesystem::path output_path = job.output.destination.path.empty() ? std::filesystem::path{} : std::filesystem::path(job.output.destination.path);
                job_result.session_result = session.render(scene, *compiled_result.value, *execution_result.value, output_path, concurrency);
                job_result.success = job_result.session_result.output_error.empty();
                if (!job_result.success) job_result.error = job_result.session_result.output_error;

                batch_result.jobs[index] = std::move(job_result);
            }
        }));
    }

    for (auto& worker : workers) {
        try { worker.get(); }
        catch (const std::exception& error) { result.diagnostics.add_error("batch.execution_failed", error.what()); }
    }

    for (const auto& job : batch_result.jobs) {
        if (job.success) ++batch_result.succeeded;
        else {
            ++batch_result.failed;
            if (!job.error.empty()) result.diagnostics.add_error("batch.job_failed", job.error);
        }
    }
    result.value = std::move(batch_result);
    return result;
}

} // namespace tachyon
