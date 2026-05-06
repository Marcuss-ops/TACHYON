#include "tachyon/core/cli.h"
#include "tachyon/core/cli_options.h"
#include "tachyon/core/spec/schema/objects/scene_spec.h"
#include "tachyon/scene/builder.h"

#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {

int g_failures = 0;

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
        std::vector<std::string> args = {"tachyon", "validate", "--cpp", "/nonexistent/path/test.cpp"};
        std::vector<char*> argv;
        for (const auto& arg : args) {
            argv.push_back(const_cast<char*>(arg.c_str()));
        }

        StreamCapture capture_out(std::cout);
        const int exit_code = tachyon::run_cli(static_cast<int>(argv.size()), argv.data());
        // exit_code should be 1 (failed to load) but NOT related to argument parsing error
        check_true(exit_code != 0, "CLI should fail on non-existent C++ file");
    }

    {
        std::vector<std::string> args = {"tachyon", "render", "--cpp", "scene.cpp", "--output-preset", "youtube_shorts"};
        std::vector<char*> argv;
        for (const auto& arg : args) {
            argv.push_back(const_cast<char*>(arg.c_str()));
        }

        const auto parsed = tachyon::parse_cli_options(static_cast<int>(argv.size()), argv.data());
        check_true(parsed.value.has_value(), "CLI parser should accept --output-preset");
        if (parsed.value.has_value()) {
            check_true(parsed.value->output_preset_id.has_value(), "CLI parser should store output preset id");
            check_true(parsed.value->output_preset_id.value() == "youtube_shorts", "CLI parser should preserve output preset id");
        }
    }

    {
        std::vector<std::string> args = {"tachyon", "output-presets", "list"};
        std::vector<char*> argv;
        for (const auto& arg : args) {
            argv.push_back(const_cast<char*>(arg.c_str()));
        }

        std::ostringstream out;
        std::ostringstream err;
        auto* old_out = std::cout.rdbuf(out.rdbuf());
        auto* old_err = std::cerr.rdbuf(err.rdbuf());
        const int exit_code = tachyon::run_cli(static_cast<int>(argv.size()), argv.data());
        std::cout.rdbuf(old_out);
        std::cerr.rdbuf(old_err);

        check_true(exit_code == 0, "CLI output-presets list should succeed");
        check_true(out.str().find("youtube_1080p_30") != std::string::npos, "CLI output-presets list should print known preset ids");
    }

    {
        std::vector<std::string> args = {"tachyon", "output-presets", "info", "youtube_4k"};
        std::vector<char*> argv;
        for (const auto& arg : args) {
            argv.push_back(const_cast<char*>(arg.c_str()));
        }

        std::ostringstream out;
        std::ostringstream err;
        auto* old_out = std::cout.rdbuf(out.rdbuf());
        auto* old_err = std::cerr.rdbuf(err.rdbuf());
        const int exit_code = tachyon::run_cli(static_cast<int>(argv.size()), argv.data());
        std::cout.rdbuf(old_out);
        std::cerr.rdbuf(old_err);

        check_true(exit_code == 0, "CLI output-presets info should succeed");
        check_true(out.str().find("youtube_4k") != std::string::npos, "CLI output-presets info should print preset name");
        check_true(out.str().find("h265") != std::string::npos, "CLI output-presets info should print preset codec");
    }

    return g_failures == 0;
}
