#include "tachyon/core/spec/schema/objects/scene_spec.h"
#include "tachyon/core/spec/schema/objects/scene_spec_core.h"
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
            check_true(!layer.shape_path.empty(), "shape path should parse");
        }
    }

    // Roundtrip test: parse -> serialize -> parse
    {
        const std::string text = R"({
            "spec_version": "1.0",
            "project": { "id": "rt_test", "name": "Roundtrip Test", "authoring_tool": "test" },
            "compositions": [
                {
                    "id": "comp1",
                    "name": "Main",
                    "width": 1920,
                    "height": 1080,
                    "duration": 120,
                    "frame_rate": { "numerator": 30, "denominator": 1 },
                    "background": "#000000",
                    "layers": [
                        {
                            "id": "layer1",
                            "type": "solid",
                            "name": "Background",
                            "blend_mode": "normal",
                            "enabled": true,
                            "visible": true,
                            "is_3d": false,
                            "is_adjustment_layer": false,
                            "motion_blur": false,
                            "start_time": 0,
                            "in_point": 0,
                            "out_point": 120,
                            "opacity": 1.0,
                            "width": 1920,
                            "height": 1080
                        },
                        {
                            "id": "layer2",
                            "type": "text",
                            "name": "Title",
                            "track_matte_layer_id": "layer1",
                            "track_matte_type": "alpha",
                            "text_content": "Hello",
                            "font_id": "Arial",
                            "alignment": "center"
                        }
                    ],
                    "camera_cuts": [
                        { "camera_id": "cam1", "start_seconds": 0, "end_seconds": 5 }
                    ]
                }
            ],
            "assets": [
                { "id": "img1", "type": "image", "source": "test.png", "alpha_mode": "premultiplied" }
            ]
        })";

        const auto parsed1 = tachyon::parse_scene_spec_json(text);
        check_true(parsed1.value.has_value(), "roundtrip: initial parse succeeds");
        if (!parsed1.value.has_value()) {
            return g_failures == 0;
        }

        const auto& scene1 = *parsed1.value;
        const json serialized = tachyon::serialize_scene_spec(scene1);
        const std::string serialized_text = serialized.dump(2);

        const auto parsed2 = tachyon::parse_scene_spec_json(serialized_text);
        check_true(parsed2.value.has_value(), "roundtrip: re-parse succeeds");
        if (!parsed2.value.has_value()) {
            return g_failures == 0;
        }

        const auto& scene2 = *parsed2.value;

        // Compare top-level fields
        check_true(scene1.version == scene2.version, "roundtrip: version matches");
        check_true(scene1.spec_version == scene2.spec_version, "roundtrip: spec_version matches");
        check_true(scene1.project.id == scene2.project.id, "roundtrip: project.id matches");
        check_true(scene1.project.name == scene2.project.name, "roundtrip: project.name matches");
        check_true(scene1.project.authoring_tool == scene2.project.authoring_tool, "roundtrip: project.authoring_tool matches");

        // Compare composition count and key fields
        check_true(scene1.compositions.size() == scene2.compositions.size(), "roundtrip: composition count matches");
        if (scene1.compositions.size() == scene2.compositions.size() && !scene1.compositions.empty()) {
            const auto& c1 = scene1.compositions[0];
            const auto& c2 = scene2.compositions[0];
            check_true(c1.id == c2.id, "roundtrip: composition id matches");
            check_true(c1.width == c2.width, "roundtrip: composition width matches");
            check_true(c1.height == c2.height, "roundtrip: composition height matches");
            check_true(c1.duration == c2.duration, "roundtrip: composition duration matches");
            check_true(c1.frame_rate.numerator == c2.frame_rate.numerator, "roundtrip: frame_rate numerator matches");
            check_true(c1.frame_rate.denominator == c2.frame_rate.denominator, "roundtrip: frame_rate denominator matches");
            check_true(c1.background == c2.background, "roundtrip: background matches");

            // Compare layers
            check_true(c1.layers.size() == c2.layers.size(), "roundtrip: layer count matches");
            if (c1.layers.size() == c2.layers.size()) {
                for (std::size_t i = 0; i < c1.layers.size(); ++i) {
                    const auto& l1 = c1.layers[i];
                    const auto& l2 = c2.layers[i];
                    check_true(l1.id == l2.id, "roundtrip: layer[" + std::to_string(i) + "].id matches");
                    check_true(l1.type == l2.type, "roundtrip: layer[" + std::to_string(i) + "].type matches");
                    check_true(l1.name == l2.name, "roundtrip: layer[" + std::to_string(i) + "].name matches");
                    check_true(l1.blend_mode == l2.blend_mode, "roundtrip: layer[" + std::to_string(i) + "].blend_mode matches");
                    check_true(l1.enabled == l2.enabled, "roundtrip: layer[" + std::to_string(i) + "].enabled matches");
                    check_true(l1.visible == l2.visible, "roundtrip: layer[" + std::to_string(i) + "].visible matches");
                    check_true(l1.opacity == l2.opacity, "roundtrip: layer[" + std::to_string(i) + "].opacity matches");
                    check_true(l1.track_matte_layer_id == l2.track_matte_layer_id, "roundtrip: layer[" + std::to_string(i) + "].track_matte_layer_id matches");
                    check_true(l1.track_matte_type == l2.track_matte_type, "roundtrip: layer[" + std::to_string(i) + "].track_matte_type matches");
                }
            }

            // Compare camera cuts
            check_true(c1.camera_cuts.size() == c2.camera_cuts.size(), "roundtrip: camera_cuts count matches");
            if (c1.camera_cuts.size() == c2.camera_cuts.size() && !c1.camera_cuts.empty()) {
                check_true(c1.camera_cuts[0].camera_id == c2.camera_cuts[0].camera_id, "roundtrip: camera_cut.camera_id matches");
                check_true(c1.camera_cuts[0].start_seconds == c2.camera_cuts[0].start_seconds, "roundtrip: camera_cut.start_seconds matches");
                check_true(c1.camera_cuts[0].end_seconds == c2.camera_cuts[0].end_seconds, "roundtrip: camera_cut.end_seconds matches");
            }
        }

        // Compare assets
        check_true(scene1.assets.size() == scene2.assets.size(), "roundtrip: asset count matches");
        if (scene1.assets.size() == scene2.assets.size() && !scene1.assets.empty()) {
            check_true(scene1.assets[0].id == scene2.assets[0].id, "roundtrip: asset.id matches");
            check_true(scene1.assets[0].type == scene2.assets[0].type, "roundtrip: asset.type matches");
            check_true(scene1.assets[0].source == scene2.assets[0].source, "roundtrip: asset.source matches");
            check_true(scene1.assets[0].alpha_mode == scene2.assets[0].alpha_mode, "roundtrip: asset.alpha_mode matches");
        }
    }

    return g_failures == 0;
}
