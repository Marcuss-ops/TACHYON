#include "tachyon/core/cli.h"
#include "tachyon/media/asset_resolution.h"
#include "tachyon/renderer2d/rasterizer.h"
#include "tachyon/runtime/core/render_graph.h"
#include "tachyon/runtime/execution/render_job.h"
#include "tachyon/runtime/execution/render_plan.h"
#include "tachyon/core/spec/scene_spec.h"

#include <nlohmann/json.hpp>

#include <algorithm>
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
    g_failures = 0;
    {
        const auto path = tests_root() / "fixtures" / "scenes" / "canonical_scene.json";
        const auto text = read_text_file(path);
        check_true(!text.empty(), "canonical scene fixture should be readable");

        const auto parsed = tachyon::parse_scene_spec_json(text);
        check_true(parsed.value.has_value(), "canonical scene spec should parse");
        if (parsed.value.has_value()) {
            const auto validation = tachyon::validate_scene_spec(*parsed.value);
            check_true(validation.ok(), "canonical scene spec should validate");

            const auto assets = tachyon::resolve_assets(*parsed.value, tests_root() / "fixtures");
            check_true(assets.value.has_value(), "canonical scene assets should resolve");
            if (assets.value.has_value()) {
                check_true(assets.value->size() == 2, "canonical scene should resolve two assets");
            }
        }
    }

    {
        const std::string text = R"({
            "spec_version": "1.0",
            "project": { "id": "proj_001", "name": "Intro" },
            "assets": [
                { "id": "logo", "type": "image", "source": "assets/logo.png" }
            ],
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
        const std::string text = R"({
            "spec_version": "1.0",
            "project": { "id": "proj_refs", "name": "Reference Scene" },
            "assets": [
                { "id": "logo", "type": "image", "source": "assets/logo.png" }
            ],
            "compositions": [
                {
                    "id": "main",
                    "name": "Main",
                    "width": 1920,
                    "height": 1080,
                    "duration": 120,
                    "frame_rate": 30,
                    "layers": [
                        { "id": "hero", "type": "image", "name": "hero_layer", "start_time": 0, "in_point": 0, "out_point": 120, "parent": "missing_parent" },
                        { "id": "matte_a", "type": "shape", "name": "Matte", "start_time": 0, "in_point": 0, "out_point": 120 },
                        { "id": "foreground", "type": "video", "name": "no_asset_here", "start_time": 0, "in_point": 0, "out_point": 120, "track_matte_layer_id": "missing_matte" },
                        { "id": "precomp_ref", "type": "precomp", "name": "Nested", "start_time": 0, "in_point": 0, "out_point": 120, "precomp_id": "missing_comp" }
                    ]
                }
            ]
        })";

        const auto parsed = tachyon::parse_scene_spec_json(text);
        check_true(parsed.value.has_value(), "reference scene should parse");
        if (parsed.value.has_value()) {
            const auto validation = tachyon::validate_scene_spec(*parsed.value);
            check_true(!validation.ok(), "invalid cross references should fail validation");
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

    {
        const auto path = tests_root() / "fixtures" / "scenes" / "canonical_scene.json";
        const auto job_path = tests_root() / "fixtures" / "jobs" / "canonical_render_job.json";
        const std::vector<std::string> args = {
            "tachyon",
            "validate",
            "--scene",
            path.string(),
            "--job",
            job_path.string(),
            "--json"
        };

        std::vector<char*> argv;
        argv.reserve(args.size());
        for (const auto& arg : args) {
            argv.push_back(const_cast<char*>(arg.c_str()));
        }

        StreamCapture capture_out(std::cout);
        StreamCapture capture_err(std::cerr);
        const int exit_code = tachyon::run_cli(static_cast<int>(argv.size()), argv.data());
        check_true(exit_code == 0, "CLI validate --json should accept canonical scene and job fixtures");
        check_true(capture_err.str().empty(), "CLI validate --json should not emit errors for canonical fixtures");

        const auto parsed_json = nlohmann::json::parse(capture_out.str());
        check_true(parsed_json["report_type"] == "validate", "validate JSON should include report type");
        check_true(parsed_json["schema_version"] == "1.0", "validate JSON should include schema version");
        check_true(parsed_json["status"] == "ok", "validate JSON should report success status");
        check_true(parsed_json["diagnostics"].is_array(), "validate JSON should include diagnostics array");
        check_true(parsed_json["scene_valid"].get<bool>(), "validate JSON should mark the scene as valid");
        check_true(parsed_json["job_valid"].get<bool>(), "validate JSON should mark the job as valid");
        check_true(parsed_json.contains("scene"), "validate JSON should contain scene");
        check_true(parsed_json.contains("assets"), "validate JSON should contain assets");
        check_true(parsed_json.contains("job"), "validate JSON should contain job");
    }

    {
        const auto path = tests_root() / "fixtures" / "scenes" / "canonical_scene.json";
        const auto job_path = tests_root() / "fixtures" / "jobs" / "canonical_render_job.json";
        const std::vector<std::string> args = {
            "tachyon",
            "inspect",
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
        check_true(exit_code == 0, "CLI inspect should accept canonical scene and job fixtures");
        check_true(capture_out.str().find("scene summary") != std::string::npos, "CLI inspect should report scene summary");
        check_true(capture_out.str().find("assets") != std::string::npos, "CLI inspect should report assets");
        check_true(capture_out.str().find("render plan") != std::string::npos, "CLI inspect should report render plan");
        check_true(capture_out.str().find("render graph") != std::string::npos, "CLI inspect should report render graph");
        check_true(capture_out.str().find("steps:") != std::string::npos, "CLI inspect should report graph step count");
        check_true(capture_err.str().empty(), "CLI inspect should not emit errors for canonical fixtures");
    }

    {
        const auto path = tests_root() / "fixtures" / "scenes" / "canonical_scene.json";
        const auto job_path = tests_root() / "fixtures" / "jobs" / "canonical_render_job.json";
        const std::vector<std::string> args = {
            "tachyon",
            "inspect",
            "--scene",
            path.string(),
            "--job",
            job_path.string(),
            "--json"
        };

        std::vector<char*> argv;
        argv.reserve(args.size());
        for (const auto& arg : args) {
            argv.push_back(const_cast<char*>(arg.c_str()));
        }

        StreamCapture capture_out(std::cout);
        StreamCapture capture_err(std::cerr);
        const int exit_code = tachyon::run_cli(static_cast<int>(argv.size()), argv.data());
        check_true(exit_code == 0, "CLI inspect --json should accept canonical scene and job fixtures");
        check_true(capture_err.str().empty(), "CLI inspect --json should not emit errors for canonical fixtures");

        const auto json_text = capture_out.str();
        const auto parsed_json = nlohmann::json::parse(json_text);
        check_true(parsed_json["report_type"] == "inspect", "inspect JSON should include report type");
        check_true(parsed_json["status"] == "ok", "inspect JSON should report success status");
        check_true(parsed_json["diagnostics"].is_array(), "inspect JSON should include diagnostics array");
        check_true(parsed_json.contains("scene"), "inspect JSON should contain scene");
        check_true(parsed_json.contains("assets"), "inspect JSON should contain assets");
        check_true(parsed_json.contains("render_plan"), "inspect JSON should contain render_plan");
        check_true(parsed_json.contains("render_graph"), "inspect JSON should contain render_graph");
        check_true(parsed_json["render_graph"].contains("steps"), "inspect JSON should contain graph steps");
        check_true(parsed_json["render_graph"]["steps"].size() >= 4, "inspect JSON should include explicit graph steps");

        const auto& steps = parsed_json["render_graph"]["steps"];
        const auto raster_step = std::find_if(steps.begin(), steps.end(), [](const auto& step) {
            return step.value("kind", "") == "rasterize-2d-frame";
        });
        check_true(raster_step != steps.end(), "inspect JSON should include a rasterize-2d-frame step");
        if (raster_step != steps.end()) {
            check_true((*raster_step)["dependencies"].is_array(), "inspect JSON should include step dependencies");
        }
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
                            "opacity_property": {
                                "keyframes": [
                                    { "time": 0, "value": 0, "easing": "ease_in" },
                                    { "time": 1, "value": 1 }
                                ]
                            },
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
            if (layer.shape_path.has_value()) {
                check_true(layer.shape_path->points.size() == 3, "shape path should keep all points");
            }
            check_true(layer.opacity_property.keyframes.size() == 2, "opacity keyframes should parse");
            if (!layer.opacity_property.keyframes.empty()) {
                check_true(layer.opacity_property.keyframes.front().easing == tachyon::animation::EasingPreset::EaseIn, "keyframe easing should parse");
            }
        }
    }

    {
        const std::string text = R"({
            "spec_version": "1.0",
            "project": { "id": "proj_video", "name": "Video Scene" },
            "compositions": [
                {
                    "id": "main",
                    "name": "Main",
                    "width": 1280,
                    "height": 720,
                    "duration": 12,
                    "frame_rate": 30,
                    "layers": [
                        {
                            "id": "clip_01",
                            "type": "video",
                            "name": "tests/fixtures/media/clip.mp4",
                            "opacity_property": {
                                "audio_band": "bass",
                                "min": 0.25,
                                "max": 0.9
                            }
                        }
                    ]
                }
            ]
        })";

        const auto parsed = tachyon::parse_scene_spec_json(text);
        check_true(parsed.value.has_value(), "video scene should parse");
        if (parsed.value.has_value()) {
            const auto& layer = parsed.value->compositions.front().layers.front();
            check_true(layer.type == "video", "video layer type should be retained");
            check_true(layer.opacity_property.audio_band.has_value(), "audio band mapping should parse");
            if (layer.opacity_property.audio_band.has_value()) {
                check_true(*layer.opacity_property.audio_band == tachyon::AudioBandType::Bass, "audio band should parse as bass");
            }
            check_true(layer.opacity_property.audio_min == 0.25, "audio band minimum should parse");
            check_true(layer.opacity_property.audio_max == 0.9, "audio band maximum should parse");

            const auto validation = tachyon::validate_scene_spec(*parsed.value);
            check_true(validation.ok(), "video scene should validate");
        }
    }

    {
        const auto path = tests_root() / "fixtures" / "scenes" / "canonical_scene.json";
        const auto job_path = tests_root() / "fixtures" / "jobs" / "canonical_render_job.json";
        const std::vector<std::string> args = {
            "tachyon",
            "render",
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
        check_true(exit_code == 0, "CLI render should accept canonical scene and job fixtures");
        check_true(capture_out.str().find("render execution plan valid") != std::string::npos, "CLI render should report graph success");
        check_true(capture_out.str().find("resolved assets: 2") != std::string::npos, "CLI render should report resolved asset count");
        check_true(capture_out.str().find("graph steps:") != std::string::npos, "CLI render should report graph step count");
        check_true(capture_out.str().find("2d runtime backend: cpu-frame-executor") != std::string::npos, "CLI render should report the runtime backend");
        check_true(capture_out.str().find("composition: main") != std::string::npos, "CLI render should print resolved composition info");
        check_true(capture_err.str().empty(), "CLI render should not emit errors for canonical fixtures");
    }

    {
        const auto path = tests_root() / "fixtures" / "scenes" / "canonical_scene.json";
        const auto job_path = tests_root() / "fixtures" / "jobs" / "canonical_render_job.json";
        const std::vector<std::string> args = {
            "tachyon",
            "render",
            "--scene",
            path.string(),
            "--job",
            job_path.string(),
            "--json"
        };

        std::vector<char*> argv;
        argv.reserve(args.size());
        for (const auto& arg : args) {
            argv.push_back(const_cast<char*>(arg.c_str()));
        }

        StreamCapture capture_out(std::cout);
        StreamCapture capture_err(std::cerr);
        const int exit_code = tachyon::run_cli(static_cast<int>(argv.size()), argv.data());
        check_true(exit_code == 0, "CLI render --json should accept canonical scene and job fixtures");
        check_true(capture_err.str().empty(), "CLI render --json should not emit errors for canonical fixtures");

        const auto parsed_json = nlohmann::json::parse(capture_out.str());
        check_true(parsed_json["report_type"] == "render", "render JSON should include report type");
        check_true(parsed_json["schema_version"] == "1.0", "render JSON should include schema version");
        check_true(parsed_json["status"] == "ok", "render JSON should report success status");
        check_true(parsed_json["diagnostics"].is_array(), "render JSON should include diagnostics array");
        check_true(parsed_json.contains("scene"), "render JSON should contain scene");
        check_true(parsed_json.contains("assets"), "render JSON should contain assets");
        check_true(parsed_json.contains("render_plan"), "render JSON should contain render_plan");
        check_true(parsed_json.contains("render_graph"), "render JSON should contain render_graph");
        check_true(parsed_json.contains("first_frame"), "render JSON should contain first_frame");
        check_true(parsed_json["first_frame"]["backend_name"] == "cpu-frame-executor", "render JSON should report the runtime backend");
    }

    return g_failures == 0;
}

bool run_render_job_tests() {
    g_failures = 0;
    const auto scene_path = tests_root() / "fixtures" / "scenes" / "canonical_scene.json";
    const auto scene_text = read_text_file(scene_path);
    check_true(!scene_text.empty(), "canonical scene fixture should be readable for planning");

    const auto scene = tachyon::parse_scene_spec_json(scene_text);
    check_true(scene.value.has_value(), "canonical scene should parse for planning");

    const auto path = tests_root() / "fixtures" / "jobs" / "canonical_render_job.json";
    const auto text = read_text_file(path);
    check_true(!text.empty(), "canonical render job fixture should be readable");

    const auto parsed = tachyon::parse_render_job_json(text);
    check_true(parsed.value.has_value(), "canonical render job should parse");
    if (parsed.value.has_value()) {
        const auto validation = tachyon::validate_render_job(*parsed.value);
        check_true(validation.ok(), "canonical render job should validate");

        if (scene.value.has_value()) {
            const auto plan = tachyon::build_render_plan(*scene.value, *parsed.value);
            check_true(plan.value.has_value(), "canonical scene and job should build a render plan");
            if (plan.value.has_value()) {
                const auto assets = tachyon::resolve_assets(*scene.value, tests_root() / "fixtures");
                check_true(assets.value.has_value(), "canonical scene assets should resolve for render planning");

                const auto execution_plan = tachyon::build_render_execution_plan(*plan.value, assets.value.has_value() ? assets.value->size() : 0);
                check_true(execution_plan.value.has_value(), "canonical render plan should build an execution plan");
                if (execution_plan.value.has_value()) {
                    check_true(execution_plan.value->resolved_asset_count == 2, "execution plan should carry the resolved asset count");
                    check_true(execution_plan.value->steps.size() >= 3, "execution plan should expose explicit graph steps");
                    check_true(!execution_plan.value->frame_tasks.empty(), "execution plan should create frame tasks");
                    check_true(execution_plan.value->frame_tasks.front().frame_number == 0, "first frame task should start at frame 0");

                    const auto raster_step = std::find_if(
                        execution_plan.value->steps.begin(),
                        execution_plan.value->steps.end(),
                        [](const auto& step) {
                            return step.kind == tachyon::RenderStepKind::Rasterize2DFrame;
                        });
                    check_true(raster_step != execution_plan.value->steps.end(), "execution plan should include a rasterize step");
                    if (raster_step != execution_plan.value->steps.end()) {
                        check_true(!raster_step->dependencies.empty(), "raster step should carry dependencies");
                    }

                    const auto first_key = tachyon::build_frame_cache_key(*plan.value, 0);
                    check_true(first_key.value == execution_plan.value->frame_tasks.front().cache_key.value, "frame task should carry the computed cache key");

                    tachyon::FrameCacheEntry matching_entry{first_key, "cached frame"};
                    check_true(tachyon::frame_cache_entry_matches(matching_entry, first_key), "cache entry should match its own key");

                    tachyon::FrameCacheEntry stale_entry{tachyon::build_frame_cache_key(*plan.value, 1), "cached frame"};
                    check_true(!tachyon::frame_cache_entry_matches(stale_entry, first_key), "different frame keys should invalidate cache entries");

                    tachyon::RenderPlan output_path_variant = *plan.value;
                    output_path_variant.output.destination.path = "out/other.mp4";
                    const auto output_path_key = tachyon::build_frame_cache_key(output_path_variant, 0);
                    check_true(output_path_key.value == first_key.value, "output path should not affect frame cache identity");

                    tachyon::RenderPlan compatibility_variant = *plan.value;
                    compatibility_variant.compatibility_mode = "legacy";
                    const auto compatibility_key = tachyon::build_frame_cache_key(compatibility_variant, 0);
                    check_true(compatibility_key.value != first_key.value, "compatibility mode should affect frame cache identity");

                    const auto rasterized = tachyon::render_frame_2d_stub(*plan.value, execution_plan.value->frame_tasks.front());
                    check_true(rasterized.backend_name == "cpu-2d-surface-rgba", "2D refined renderer should report its backend");
                    check_true(rasterized.layer_count == 1, "2D stub renderer should reflect composition layer count");
                    check_true(rasterized.estimated_draw_ops == 5, "2D stub renderer should derive deterministic draw ops");
                }

                check_true(plan.value->composition.id == "main", "render plan should resolve the target composition");
                check_true(plan.value->composition.layer_count == 1, "render plan should carry the composition summary");
                check_true(plan.value->composition.width == 1920, "render plan should preserve composition width");
                check_true(plan.value->output.destination.path == "out/intro.mp4", "render plan should keep output destination");
            }
        }
    }

    {
        tachyon::SceneSpec broken_scene;
        broken_scene.spec_version = "1.0";
        broken_scene.project.id = "proj_001";
        broken_scene.project.name = "Broken";
        tachyon::CompositionSpec broken_composition;
        broken_composition.id = "main";
        broken_composition.name = "Main";
        broken_composition.width = 1920;
        broken_composition.height = 1080;
        broken_composition.duration = 120.0;
        broken_composition.frame_rate = {30, 1};
        broken_scene.compositions.push_back(broken_composition);
        broken_scene.assets.push_back(tachyon::AssetSpec{"missing_logo", "image", "assets/missing-logo.png"});

        const auto assets = tachyon::resolve_assets(broken_scene, tests_root() / "fixtures");
        check_true(!assets.value.has_value(), "missing asset should fail resolution");
        check_true(!assets.diagnostics.ok(), "missing asset should emit diagnostics");
    }

    {
        const std::string invalid_text = R"({
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

        const auto invalid = tachyon::parse_render_job_json(invalid_text);
        check_true(invalid.value.has_value(), "invalid render job should still parse");
        if (invalid.value.has_value()) {
            const auto validation = tachyon::validate_render_job(*invalid.value);
            check_true(!validation.ok(), "invalid render job should fail validation");
        }
    }

    {
        const auto scene_text_again = read_text_file(tests_root() / "fixtures" / "scenes" / "canonical_scene.json");
        const auto scene_again = tachyon::parse_scene_spec_json(scene_text_again);
        const auto job_text_again = read_text_file(tests_root() / "fixtures" / "jobs" / "canonical_render_job.json");
        const auto job_again = tachyon::parse_render_job_json(job_text_again);

        if (scene_again.value.has_value() && job_again.value.has_value()) {
            tachyon::RenderJob unresolved = *job_again.value;
            unresolved.composition_target = "missing";
            const auto plan = tachyon::build_render_plan(*scene_again.value, unresolved);
            check_true(!plan.value.has_value(), "missing composition target should fail render plan construction");

            if (plan.diagnostics.ok()) {
                check_true(false, "missing composition target should add a diagnostic");
            }
        }
    }

    return g_failures == 0;
}
