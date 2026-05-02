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
    const char* /*scene_json*/,
    const char* /*job_json*/,
    const char* /*output_path*/,
    char* error_buf,
    int error_buf_size) {
    return tachyon::fail(error_buf, error_buf_size, "tachyon_render_from_json is DEPRECATED and disabled. JSON scene loading is no longer supported.", 5);
}
