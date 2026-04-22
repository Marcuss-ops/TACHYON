#include "tachyon/runtime/execution/batch/batch_runner.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace {

int g_failures = 0;

void check_true(bool condition, const std::string& message) {
    if (!condition) {
        ++g_failures;
        std::cerr << "FAIL: " << message << '\n';
    }
}

std::filesystem::path tests_root() {
    return std::filesystem::path(TACHYON_TESTS_SOURCE_DIR);
}

} // namespace

bool run_render_batch_tests() {
    using namespace tachyon;

    g_failures = 0;

    const std::filesystem::path output_dir = tests_root() / "output" / "batch";
    std::filesystem::remove_all(output_dir);
    std::filesystem::create_directories(output_dir);

    const std::filesystem::path batch_file = output_dir / "batch_jobs.json";
    std::ofstream batch_out(batch_file, std::ios::binary | std::ios::trunc);
    batch_out
        << "{\n"
        << "  \"workers\": 2,\n"
        << "  \"jobs\": [\n"
        << "    {\n"
        << "      \"scene\": \"../../fixtures/scenes/scene_graph_minimal.json\",\n"
        << "      \"job\": \"../../fixtures/jobs/png_sequence_render_job.json\",\n"
        << "      \"out\": \"first.png\"\n"
        << "    },\n"
        << "    {\n"
        << "      \"scene\": \"../../fixtures/scenes/scene_graph_minimal.json\",\n"
        << "      \"job\": \"../../fixtures/jobs/png_sequence_render_job.json\",\n"
        << "      \"out\": \"second.png\"\n"
        << "    }\n"
        << "  ]\n"
        << "}\n";
    batch_out.close();

    const auto parsed = parse_render_batch_file(batch_file);
    check_true(parsed.value.has_value(), "Batch file parses");
    if (!parsed.value.has_value()) {
        return false;
    }

    const auto validation = validate_render_batch_spec(*parsed.value);
    check_true(validation.ok(), "Batch spec validates");
    if (!validation.ok()) {
        return false;
    }

    const auto result = run_render_batch(*parsed.value, 2);
    check_true(result.value.has_value(), "Batch run returns a value");
    if (!result.value.has_value()) {
        return false;
    }

    for (std::size_t index = 0; index < result.value->jobs.size(); ++index) {
        if (!result.value->jobs[index].success) {
            std::cerr << "batch job " << index << " error: " << result.value->jobs[index].error << '\n';
            std::cerr << "output error: " << result.value->jobs[index].session_result.output_error << '\n';
        }
    }

    check_true(result.value->jobs.size() == 2, "Batch produces two job results");
    check_true(result.value->succeeded == 2, "Both batch jobs succeed");
    check_true(result.value->failed == 0, "No batch jobs fail");
    check_true(std::filesystem::exists(output_dir / "first_000001.png"), "First batch output exists");
    check_true(std::filesystem::exists(output_dir / "second_000001.png"), "Second batch output exists");
    check_true(result.value->jobs[0].session_result.frames.size() == 2, "First batch job renders two frames");
    check_true(result.value->jobs[1].session_result.frames.size() == 2, "Second batch job renders two frames");

    return g_failures == 0;
}
