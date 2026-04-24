#include "tachyon/core/c_api.h"

#include "tachyon/core/spec/compilation/scene_compiler.h"
#include "tachyon/core/spec/schema/objects/scene_spec_core.h"
#include "tachyon/runtime/execution/jobs/render_job.h"
#include "tachyon/runtime/execution/planning/render_plan.h"
#include "tachyon/runtime/execution/session/render_session.h"

#include <cstring>
#include <algorithm>
#include <filesystem>
#include <string>
#include <thread>
#include <utility>

namespace tachyon {
namespace {

void write_error(char* error_buf, int error_buf_size, const std::string& message) {
    if (error_buf == nullptr || error_buf_size <= 0) {
        return;
    }
    const int usable = error_buf_size - 1;
    if (usable <= 0) {
        return;
    }
    const int count = std::min<int>(usable, static_cast<int>(message.size()));
    std::memcpy(error_buf, message.data(), static_cast<std::size_t>(count));
    error_buf[count] = '\0';
}

int fail(char* error_buf, int error_buf_size, const std::string& message, int code) {
    write_error(error_buf, error_buf_size, message);
    return code;
}

}  // namespace

}  // namespace tachyon

extern "C" int tachyon_render_from_json(
    const char* scene_json,
    const char* job_json,
    const char* output_path,
    char* error_buf,
    int error_buf_size) {
    using namespace tachyon;

    try {
        if (scene_json == nullptr || job_json == nullptr) {
            return fail(error_buf, error_buf_size, "scene_json and job_json are required", 1);
        }

        const auto scene_parsed = parse_scene_spec_json(scene_json);
        if (!scene_parsed.ok() || !scene_parsed.value.has_value()) {
            std::string message = "failed to parse scene json";
            if (!scene_parsed.diagnostics.diagnostics.empty()) {
                message += ": " + scene_parsed.diagnostics.diagnostics.front().message;
            }
            return fail(error_buf, error_buf_size, message, 1);
        }

        const auto job_parsed = parse_render_job_json(job_json);
        if (!job_parsed.ok() || !job_parsed.value.has_value()) {
            std::string message = "failed to parse job json";
            if (!job_parsed.diagnostics.diagnostics.empty()) {
                message += ": " + job_parsed.diagnostics.diagnostics.front().message;
            }
            return fail(error_buf, error_buf_size, message, 1);
        }

        SceneSpec scene = std::move(*scene_parsed.value);
        RenderJob job = std::move(*job_parsed.value);

        if (output_path != nullptr && output_path[0] != '\0') {
            job.output.destination.path = output_path;
        }
        if (job.output.destination.path.empty()) {
            return fail(error_buf, error_buf_size, "an output path is required", 2);
        }

        const auto scene_validation = validate_scene_spec(scene);
        if (!scene_validation.ok()) {
            return fail(error_buf, error_buf_size, "scene validation failed", 2);
        }
        const auto job_validation = validate_render_job(job);
        if (!job_validation.ok()) {
            return fail(error_buf, error_buf_size, "render job validation failed", 2);
        }

        SceneCompiler compiler;
        const auto compiled = compiler.compile(scene);
        if (!compiled.ok() || !compiled.value.has_value()) {
            return fail(error_buf, error_buf_size, "scene compilation failed", 3);
        }

        const auto plan = build_render_plan(scene, job);
        if (!plan.ok() || !plan.value.has_value()) {
            return fail(error_buf, error_buf_size, "render plan build failed", 3);
        }
        const auto execution = build_render_execution_plan(*plan.value, scene.assets.size());
        if (!execution.ok() || !execution.value.has_value()) {
            return fail(error_buf, error_buf_size, "execution plan build failed", 3);
        }

        RenderSession session;
        const std::size_t worker_count = std::max<std::size_t>(1U, std::thread::hardware_concurrency());
        const auto result = session.render(scene, *compiled.value, *execution.value, job.output.destination.path, worker_count);
        if (!result.output_error.empty()) {
            return fail(error_buf, error_buf_size, result.output_error, 4);
        }
        return 0;
    } catch (const std::exception& ex) {
        return fail(error_buf, error_buf_size, ex.what(), 5);
    } catch (...) {
        return fail(error_buf, error_buf_size, "unknown tachyon c api error", 5);
    }
}
