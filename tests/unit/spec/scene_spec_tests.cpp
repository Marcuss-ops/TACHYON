#include "tachyon/core/cli.h"
#include "tachyon/runtime/render_job.h"
#include "tachyon/spec/scene_spec.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {

int g_failures = 0;

const std::filesystem::path& tests_root() {
    static const std::filesystem::path root = TACHYON_TESTS_SOURCE_DIR;
    return root;
}

std::string read_text_file(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::in | std::ios::binary);
    if (!input.is_open()) {
        return {};
    }

    std::ostringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
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
        : stream(target)
        , previous(target.rdbuf(buffer.rdbuf())) {
    }

    ~StreamCapture() {
        stream.rdbuf(previous);
    }

    [[nodiscard]] std::string str() const {
        return buffer.str();
    }
};

} // namespace

bool run_scene_spec_tests() {
    {
        const auto path = tests_root() / "fixtures" / "scenes" / "canonical_scene.json";
        const auto text = read_text_file(path);
        check_true(!text.empty(), "canonical scene fixture should be readable");

        const auto parsed = tachyon::parse_scene_spec_json(text);
        check_true(parsed.value.has_value(), "canonical scene spec should parse");
        if (parsed.value.has_value()) {
            const auto validation = tachyon::validate_scene_spec(*parsed.value);
            check_true(validation.ok(), "canonical scene spec should validate");
        }
    }

    {
        const std::string text = R"({
            "spec_version": "1.0",
            "project": { "id": "proj_001", "name": "Intro" },
            "compositions": [
                {
                    "id": "main",
                    "name": "Main",
                    "width": 1920,
                    "height": 1080,
                    "duration": 120,
                    "frame_rate": 30,
                    "layers": [
                        { "id": "title", "type": "text", "name": "Title", "start_time": 0, "in_point": 0, "out_point": 120 },
                        { "id": "title", "type": "text", "name": "Title 2", "start_time": 0, "in_point": 0, "out_point": 120 }
                    ]
                }
            ]
        })";

        const auto parsed = tachyon::parse_scene_spec_json(text);
        check_true(parsed.value.has_value(), "duplicate-layer scene should parse");
        if (parsed.value.has_value()) {
            const auto validation = tachyon::validate_scene_spec(*parsed.value);
            check_true(!validation.ok(), "duplicate layer ids should fail validation");
        }
    }

    {
        const auto path = tests_root() / "fixtures" / "scenes" / "canonical_scene.json";
        const auto job_path = tests_root() / "fixtures" / "jobs" / "canonical_render_job.json";
        const std::vector<std::string> args = {
            "tachyon",
            "validate",
            "--scene",
            path.string(),
            "--job",
            job_path.string()
        };

        std::vector<char*> argv;
        argv.reserve(args.size());
        for (const auto& arg : args) {
            argv.push_back(const_cast<char*>(arg.c_str()));
        }

        StreamCapture capture_out(std::cout);
        StreamCapture capture_err(std::cerr);
        const int exit_code = tachyon::run_cli(static_cast<int>(argv.size()), argv.data());
        check_true(exit_code == 0, "CLI validate should accept canonical scene and job fixtures");
        check_true(capture_out.str().find("scene spec valid") != std::string::npos, "CLI validate should report scene success");
        check_true(capture_out.str().find("render job valid") != std::string::npos, "CLI validate should report job success");
        check_true(capture_err.str().empty(), "CLI validate should not emit errors for canonical fixtures");
    }

    return g_failures == 0;
}

bool run_render_job_tests() {
    const auto path = tests_root() / "fixtures" / "jobs" / "canonical_render_job.json";
    const auto text = read_text_file(path);
    check_true(!text.empty(), "canonical render job fixture should be readable");

    const auto parsed = tachyon::parse_render_job_json(text);
    check_true(parsed.value.has_value(), "canonical render job should parse");
    if (parsed.value.has_value()) {
        const auto validation = tachyon::validate_render_job(*parsed.value);
        check_true(validation.ok(), "canonical render job should validate");
    }

    {
        const std::string text = R"({
            "job_id": "job_invalid",
            "scene_ref": "scene.json",
            "composition_target": "main",
            "frame_range": { "start": 10, "end": 0 },
            "output": {
                "destination": { "path": "out/invalid.mp4", "overwrite": false },
                "profile": {
                    "name": "broken",
                    "class": "delivery",
                    "container": "mp4",
                    "video": {
                        "codec": "h264",
                        "pixel_format": "yuv420p",
                        "rate_control_mode": "crf",
                        "crf": 18
                    },
                    "audio": {
                        "mode": "encode",
                        "codec": "aac"
                    },
                    "buffering": {
                        "strategy": "pipe"
                    },
                    "color": {
                        "transfer": "bt709",
                        "range": "tv"
                    }
                }
            }
        })";

        const auto invalid = tachyon::parse_render_job_json(text);
        check_true(invalid.value.has_value(), "invalid render job should still parse");
        if (invalid.value.has_value()) {
            const auto validation = tachyon::validate_render_job(*invalid.value);
            check_true(!validation.ok(), "invalid render job should fail validation");
        }
    }

    return g_failures == 0;
}
