#pragma once

#include "tachyon/runtime/diagnostics.h"
#include "tachyon/runtime/render_session.h"

#include <cstddef>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace tachyon {

struct RenderBatchItem {
    std::filesystem::path scene_path;
    std::filesystem::path job_path;
    std::optional<std::filesystem::path> output_override;
};

struct RenderBatchSpec {
    std::size_t worker_count{1};
    std::vector<RenderBatchItem> jobs;
};

struct RenderBatchJobResult {
    RenderBatchItem request;
    bool success{false};
    std::string error;
    RenderSessionResult session_result;
};

struct RenderBatchResult {
    std::vector<RenderBatchJobResult> jobs;
    std::size_t succeeded{0};
    std::size_t failed{0};
};

ParseResult<RenderBatchSpec> parse_render_batch_json(const std::string& text);
ParseResult<RenderBatchSpec> parse_render_batch_file(const std::filesystem::path& path);
ValidationResult validate_render_batch_spec(const RenderBatchSpec& spec);
ResolutionResult<RenderBatchResult> run_render_batch(const RenderBatchSpec& spec, std::size_t worker_count = 1);

} // namespace tachyon
