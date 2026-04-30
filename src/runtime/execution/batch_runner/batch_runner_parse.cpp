#include "batch_runner_internal.h"
#include <fstream>
#include <sstream>

namespace tachyon {

RenderBatchSpec parse_batch_spec(const json& root, const std::filesystem::path& base_path, DiagnosticBag& diagnostics) {
    RenderBatchSpec spec;
    if (root.contains("workers")) {
        std::int64_t workers = 0;
        if (read_number(root, "workers", workers) && workers > 0) {
            spec.worker_count = static_cast<std::size_t>(workers);
        } else {
            diagnostics.add_error("batch.workers_invalid", "workers must be a positive integer", "workers");
        }
    }

    if (!root.contains("jobs") || !root.at("jobs").is_array()) {
        diagnostics.add_error("batch.jobs_missing", "jobs array is required", "jobs");
        return spec;
    }

    const auto& jobs = root.at("jobs");
    for (std::size_t index = 0; index < jobs.size(); ++index) {
        const auto& job = jobs.at(index);
        const std::string item_path = "jobs[" + std::to_string(index) + "]";
        if (!job.is_object()) {
            diagnostics.add_error("batch.job_invalid", "job entry must be an object", item_path);
            continue;
        }

        RenderBatchItem item;
        std::string scene_value;
        std::string job_value;
        if (read_string(job, "scene", scene_value) || read_string(job, "scene_path", scene_value)) {}
        else diagnostics.add_error("batch.job.scene_missing", "scene path is required", item_path + ".scene");

        if (read_string(job, "job", job_value) || read_string(job, "job_path", job_value)) {}
        else diagnostics.add_error("batch.job.job_missing", "job path is required", item_path + ".job");

        if (job.contains("out") && job.at("out").is_string()) {
            item.output_override = resolve_relative_path(base_path, job.at("out").get<std::string>());
        } else if (job.contains("output_override") && job.at("output_override").is_string()) {
            item.output_override = resolve_relative_path(base_path, job.at("output_override").get<std::string>());
        }

        if (!scene_value.empty()) item.scene_path = resolve_relative_path(base_path, scene_value);
        if (!job_value.empty()) item.job_path = resolve_relative_path(base_path, job_value);

        spec.jobs.push_back(std::move(item));
    }
    return spec;
}

ParseResult<RenderBatchSpec> parse_render_batch_json(const std::string& text) {
    ParseResult<RenderBatchSpec> result;
    json root;
    try { root = json::parse(text); }
    catch (const json::parse_error& error) {
        result.diagnostics.add_error("batch.json.parse_failed", error.what());
        return result;
    }
    if (!root.is_object()) {
        result.diagnostics.add_error("batch.json.root_invalid", "batch root must be an object");
        return result;
    }
    const RenderBatchSpec spec = parse_batch_spec(root, {}, result.diagnostics);
    if (result.diagnostics.ok()) result.value = spec;
    return result;
}

ParseResult<RenderBatchSpec> parse_render_batch_file(const std::filesystem::path& path) {
    ParseResult<RenderBatchSpec> result;
    std::ifstream input(path, std::ios::in | std::ios::binary);
    if (!input.is_open()) {
        result.diagnostics.add_error("batch.file.open_failed", "failed to open batch file: " + path.string(), path.string());
        return result;
    }
    std::stringstream buffer;
    buffer << input.rdbuf();
    json root;
    try { root = json::parse(buffer.str()); }
    catch (const json::parse_error& error) {
        result.diagnostics.add_error("batch.json.parse_failed", error.what(), path.string());
        return result;
    }
    if (!root.is_object()) {
        result.diagnostics.add_error("batch.json.root_invalid", "batch root must be an object", path.string());
        return result;
    }
    const RenderBatchSpec spec = parse_batch_spec(root, path.parent_path(), result.diagnostics);
    if (result.diagnostics.ok()) result.value = spec;
    return result;
}

} // namespace tachyon
