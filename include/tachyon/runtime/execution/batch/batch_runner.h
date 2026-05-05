#pragma once

#include "tachyon/runtime/core/diagnostics/diagnostics.h"
#include "tachyon/runtime/execution/session/render_session.h"

#include <cstddef>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include "tachyon/runtime/telemetry/batch_telemetry_aggregator.h"

namespace tachyon {

struct RetryPolicy {
    int max_attempts{1};
    double backoff_seconds{2.0};
    // In a real system, we'd filter by RenderErrorCode here
};

struct RenderBatchItem {
    std::filesystem::path scene_path;
    std::filesystem::path job_path;
    std::optional<std::filesystem::path> output_override;
    std::optional<double> timeout_seconds;
    RetryPolicy retry_policy;
};

struct RenderBatchSpec {
    std::size_t worker_count{1};
    std::vector<RenderBatchItem> jobs;
    std::string batch_id;
    std::optional<double> default_timeout_seconds;
};

struct RenderBatchJobResult {
    RenderBatchItem request;
    bool success{false};
    std::string error;
    RenderSessionResult session_result;
    RenderTelemetryRecord telemetry;
};

struct RenderBatchResult {
    std::vector<RenderBatchJobResult> jobs;
    std::size_t succeeded{0};
    std::size_t failed{0};
    BatchTelemetrySummary summary;
};

ValidationResult validate_render_batch_spec(const RenderBatchSpec& spec);
ResolutionResult<RenderBatchResult> run_render_batch(const RenderBatchSpec& spec, std::size_t worker_count = 1);

} // namespace tachyon
