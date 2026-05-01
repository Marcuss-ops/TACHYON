#include <gtest/gtest.h>

#include "tachyon/core/cli.h"
#include "tachyon/core/spec/cpp_scene_loader.h"
#include "tachyon/core/spec/schema/objects/scene_spec.h"
#include "tachyon/scene/builder.h"

#include <filesystem>
#include <iostream>
#include <sstream>
#include <vector>

namespace {

int g_failures = 0;

const std::filesystem::path& tests_root() {
    static const std::filesystem::path root = TACHYON_TESTS_SOURCE_DIR;
    return root;
}

void check_true(bool condition, const std::string& message) {
    if (!condition) {
        ++g_failures;
        std::cerr << "FAIL: " << message << '\n';
    }
}

struct StreamCapture {
    std::ostream& stream;
    std::streambuf* previous;
    std::ostringstream buffer;

    explicit StreamCapture(std::ostream& target)
        : stream(target), previous(target.rdbuf(buffer.rdbuf())) {}

    ~StreamCapture() {
        stream.rdbuf(previous);
    }

    std::string str() const {
        return buffer.str();
    }
};

} // namespace

bool run_cli_tests() {
    g_failures = 0;

    {
        tachyon::SceneSpec scene;
        scene.project.id = "proj_cli_test";
        scene.project.name = "CLI Test";
        scene.compositions.push_back(
            tachyon::scene::Composition("main")
                .size(1920, 1080)
                .fps(30)
                .duration(10.0)
                .build());
        check_true(!scene.compositions.empty(), "CLI test scene should have compositions");
    }

    {
        const auto path = tests_root() / "fixtures" / "scenes" / "canonical_scene.json";
        const auto job_path = tests_root() / "fixtures" / "jobs" / "canonical_render_job.json";
        std::vector<std::string> args = {
            "tachyon", "validate", "--scene", path.string(), "--job", job_path.string()
        };
        std::vector<char*> argv;
        for (const auto& arg : args) {
            argv.push_back(const_cast<char*>(arg.c_str()));
        }

        StreamCapture capture_out(std::cout);
        const int exit_code = tachyon::run_cli(static_cast<int>(argv.size()), argv.data());
        check_true(exit_code == 0, "CLI validate should accept canonical fixtures");
    }

    {
        std::vector<std::string> args = {"tachyon", "validate", "--cpp", "/nonexistent/path/test.cpp"};
        std::vector<char*> argv;
        for (const auto& arg : args) {
            argv.push_back(const_cast<char*>(arg.c_str()));
        }

        StreamCapture capture_out(std::cout);
        const int exit_code = tachyon::run_cli(static_cast<int>(argv.size()), argv.data());
        check_true(exit_code != 1, "CLI should recognize --cpp flag (exit 1 = arg parse error)");
    }

    return g_failures == 0;
}
