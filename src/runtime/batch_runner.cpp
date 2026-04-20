#include "tachyon/runtime/batch_runner.h"

#include "tachyon/media/asset_resolution.h"
#include "tachyon/runtime/render_graph.h"
#include "tachyon/runtime/render_job.h"
#include "tachyon/runtime/render_plan.h"
#include "tachyon/core/spec/scene_spec.h"

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <future>
#include <exception>
#include <nlohmann/json.hpp>
#include <sstream>
#include <string>

namespace tachyon {
namespace {

using json = nlohmann::json;

std::filesystem::path resolve_relative_path(const std::filesystem::path& base, const std::filesystem::path& value) {
    if (value.empty() || value.is_absolute()) {
        return value;
    }
    return base.empty() ? value : base / value;
}

std::filesystem::path scene_asset_root(const std::filesystem::path& scene_path) {
    const auto scene_dir = scene_path.parent_path();
    if (scene_dir.empty()) {
        return scene_dir;
    }

    const auto folder_name = scene_dir.filename().string();
    if ((folder_name == "scenes" || folder_name == "project") && !scene_dir.parent_path().empty()) {
        return scene_dir.parent_path();
    }

    return scene_dir;
}

bool read_string(const json& object, const char* key, std::string& out) {
    if (!object.contains(key) || !object.at(key).is_string()) {
        return false;
    }
    out = object.at(key).get<std::string>();
    return true;
}

bool read_int(const json& object, const char* key, std::int64_t& out) {
    if (!object.contains(key) || !object.at(key).is_number_integer()) {
        return false;
    }
    out = object.at(key).get<std::int64_t>();
    return true;
}

RenderBatchSpec parse_batch_spec(const json& root, const std::filesystem::path& base_path, DiagnosticBag& diagnostics) {
    RenderBatchSpec spec;

    if (root.contains("workers")) {
        std::int64_t workers = 0;
        if (read_int(root, "workers", workers) && workers > 0) {
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
        std::string output_value;

        if (read_string(job, "scene", scene_value) || read_string(job, "scene_path", scene_value)) {
            // no-op, handled below
        } else {
            diagnostics.add_error("batch.job.scene_missing", "scene path is required", item_path + ".scene");
        }

        if (read_string(job, "job", job_value) || read_string(job, "job_path", job_value)) {
            // no-op, handled below
        } else {
            diagnostics.add_error("batch.job.job_missing", "job path is required", item_path + ".job");
        }

        if (job.contains("out") && job.at("out").is_string()) {
            output_value = job.at("out").get<std::string>();
            item.output_override = resolve_relative_path(base_path, output_value);
        } else if (job.contains("output_override") && job.at("output_override").is_string()) {
            output_value = job.at("output_override").get<std::string>();
            item.output_override = resolve_relative_path(base_path, output_value);
        }

        if (!scene_value.empty()) {
            item.scene_path = resolve_relative_path(base_path, scene_value);
        }
        if (!job_value.empty()) {
            item.job_path = resolve_relative_path(base_path, job_value);
        }

        spec.jobs.push_back(std::move(item));
    }

    return spec;
}

bool load_scene_context(
    const std::filesystem::path& scene_path,
    SceneSpec& scene,
    AssetResolutionTable& assets,
    DiagnosticBag& diagnostics) {

    const auto parsed = parse_scene_spec_file(scene_path);
    if (!parsed.value.has_value()) {
        diagnostics.append(parsed.diagnostics);
        return false;
    }

    const auto validation = validate_scene_spec(*parsed.value);
    if (!validation.ok()) {
        diagnostics.append(validation.diagnostics);
        return false;
    }

    const auto resolved_assets = resolve_assets(*parsed.value, scene_asset_root(scene_path));
    if (!resolved_assets.value.has_value()) {
        diagnostics.append(resolved_assets.diagnostics);
        return false;
    }

    scene = *parsed.value;
    assets = *resolved_assets.value;
    return true;
}

bool load_render_job(
    const std::filesystem::path& job_path,
    RenderJob& job,
    DiagnosticBag& diagnostics) {

    const auto parsed = parse_render_job_file(job_path);
    if (!parsed.value.has_value()) {
        diagnostics.append(parsed.diagnostics);
        return false;
    }

    const auto validation = validate_render_job(*parsed.value);
    if (!validation.ok()) {
        diagnostics.append(validation.diagnostics);
        return false;
    }

    job = *parsed.value;
    return true;
}

std::string summarize_diagnostics(const DiagnosticBag& diagnostics, const std::string& fallback) {
    if (diagnostics.diagnostics.empty()) {
        return fallback;
    }

    const auto& first = diagnostics.diagnostics.front();
    return first.code + ": " + first.message;
}

} // namespace

ParseResult<RenderBatchSpec> parse_render_batch_json(const std::string& text) {
    ParseResult<RenderBatchSpec> result;

    json root;
    try {
        root = json::parse(text);
    } catch (const json::parse_error& error) {
        result.diagnostics.add_error("batch.json.parse_failed", error.what());
        return result;
    }

    if (!root.is_object()) {
        result.diagnostics.add_error("batch.json.root_invalid", "batch root must be an object");
        return result;
    }

    const RenderBatchSpec spec = parse_batch_spec(root, {}, result.diagnostics);
    if (result.diagnostics.ok()) {
        result.value = spec;
    }
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
    try {
        root = json::parse(buffer.str());
    } catch (const json::parse_error& error) {
        result.diagnostics.add_error("batch.json.parse_failed", error.what(), path.string());
        return result;
    }

    if (!root.is_object()) {
        result.diagnostics.add_error("batch.json.root_invalid", "batch root must be an object", path.string());
        return result;
    }

    const RenderBatchSpec spec = parse_batch_spec(root, path.parent_path(), result.diagnostics);
    if (result.diagnostics.ok()) {
        result.value = spec;
    }
    return result;
}

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
                if (index >= spec.jobs.size()) {
                    return;
                }

                const RenderBatchItem& request = spec.jobs[index];
                RenderBatchJobResult job_result;
                job_result.request = request;

                DiagnosticBag diagnostics;
                SceneSpec scene;
                AssetResolutionTable assets;
                if (!load_scene_context(request.scene_path, scene, assets, diagnostics)) {
                    job_result.error = summarize_diagnostics(diagnostics, "failed to load scene");
                    batch_result.jobs[index] = std::move(job_result);
                    continue;
                }

                RenderJob job;
                if (!load_render_job(request.job_path, job, diagnostics)) {
                    job_result.error = summarize_diagnostics(diagnostics, "failed to load job");
                    batch_result.jobs[index] = std::move(job_result);
                    continue;
                }

                if (request.output_override.has_value()) {
                    job.output.destination.path = request.output_override->string();
                }

                const auto plan_result = build_render_plan(scene, job);
                if (!plan_result.value.has_value()) {
                    job_result.error = "failed to build render plan";
                    batch_result.jobs[index] = std::move(job_result);
                    continue;
                }

                const auto execution_result = build_render_execution_plan(*plan_result.value, assets.size());
                if (!execution_result.value.has_value()) {
                    job_result.error = "failed to build execution plan";
                    batch_result.jobs[index] = std::move(job_result);
                    continue;
                }

                RenderSession session;
                const std::filesystem::path output_path = job.output.destination.path.empty()
                    ? std::filesystem::path{}
                    : std::filesystem::path(job.output.destination.path);
                job_result.session_result = session.render(scene, *execution_result.value, output_path, concurrency);
                job_result.success = job_result.session_result.output_error.empty();
                if (!job_result.success) {
                    job_result.error = job_result.session_result.output_error;
                }

                batch_result.jobs[index] = std::move(job_result);
            }
        }));
    }

    for (auto& worker : workers) {
        try {
            worker.get();
        } catch (const std::exception& error) {
            result.diagnostics.add_error("batch.execution_failed", error.what());
        }
    }

    for (const auto& job : batch_result.jobs) {
        if (job.success) {
            ++batch_result.succeeded;
        } else {
            ++batch_result.failed;
            if (!job.error.empty()) {
                result.diagnostics.add_error("batch.job_failed", job.error);
            }
        }
    }

    result.value = std::move(batch_result);
    return result;
}

} // namespace tachyon
