#include "tachyon/core/spec/scene_spec.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iostream>

namespace {

int g_failures = 0;

const std::filesystem::path& tests_root() {
    static const std::filesystem::path root = TACHYON_TESTS_SOURCE_DIR;
    return root;
}

std::string read_text_file(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::in | std::ios::binary);
    if (!input.is_open()) return {};
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

} // namespace

bool run_scene_spec_parsing_tests() {
    g_failures = 0;
    {
        const auto path = tests_root() / "fixtures" / "scenes" / "canonical_scene.json";
        const auto text = read_text_file(path);
        check_true(!text.empty(), "canonical scene fixture should be readable");

        const auto parsed = tachyon::parse_scene_spec_json(text);
        check_true(parsed.value.has_value(), "canonical scene spec should parse");
    }

    {
        const std::string text = R"({
            "spec_version": "1.0",
            "project": { "id": "proj_shape", "name": "Shape Scene" },
            "compositions": [
                {
                    "id": "main",
                    "name": "Main",
                    "width": 320,
                    "height": 180,
                    "duration": 10,
                    "frame_rate": 30,
                    "layers": [
                        {
                            "id": "shape_01",
                            "type": "shape",
                            "name": "Triangle",
                            "shape_path": {
                                "closed": true,
                                "points": [[0, 0], [40, 0], [20, 30]]
                            }
                        }
                    ]
                }
            ]
        })";

        const auto parsed = tachyon::parse_scene_spec_json(text);
        check_true(parsed.value.has_value(), "shape scene should parse");
        if (parsed.value.has_value()) {
            const auto& layer = parsed.value->compositions.front().layers.front();
            check_true(layer.shape_path.has_value(), "shape path should parse");
        }
    }

    return g_failures == 0;
}
