#include "batch_runner_internal.h"

namespace tachyon {

ValidationResult validate_render_batch_spec(const RenderBatchSpec& spec) {
    ValidationResult result;
    if (spec.worker_count == 0) {
        result.diagnostics.add_error("batch.workers_invalid", "worker_count must be greater than zero", "workers");
    }
    if (spec.jobs.empty()) {
        result.diagnostics.add_error("batch.jobs_empty", "at least one batch job is required", "jobs");
    }
    for (std::size_t index = 0; index < spec.jobs.size(); ++index) {
        const auto& job = spec.jobs[index];
        if (job.scene_path.empty()) {
            result.diagnostics.add_error("batch.job.scene_missing", "scene path is required", "jobs[" + std::to_string(index) + "].scene");
        }
        if (job.job_path.empty()) {
            result.diagnostics.add_error("batch.job.job_missing", "job path is required", "jobs[" + std::to_string(index) + "].job");
        }
    }
    return result;
}

} // namespace tachyon
