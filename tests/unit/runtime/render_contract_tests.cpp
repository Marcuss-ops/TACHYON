#include "tachyon/runtime/render_graph.h"
#include "tachyon/runtime/render_job.h"
#include "tachyon/runtime/render_plan.h"
#include "tachyon/core/scene/evaluator.h"
#include "tachyon/core/spec/scene_spec.h"

#include <cmath>
#include <iostream>
#include <string>

#include <nlohmann/json.hpp>

namespace {

int g_failures = 0;

void check_true(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "[FAIL] " << message << '\n';
        ++g_failures;
    }
}

std::string render_job_json(std::string quality_tier, bool motion_blur_enabled, std::int64_t samples, std::string motion_blur_curve = "box") {
    nlohmann::json root = {
        {"job_id", "job_01"},
        {"scene_ref", "scene_01"},
        {"composition_target", "main"},
        {"quality_tier", quality_tier},
        {"compositing_alpha_mode", "premultiplied"},
        {"working_space", "linear_rec709"},
        {"motion_blur_enabled", motion_blur_enabled},
        {"motion_blur_samples", samples},
        {"motion_blur_shutter_angle", 180.0},
        {"motion_blur_curve", motion_blur_curve},
        {"output", {
            {"destination", {
                {"path", "tests/output/render_contract.mp4"},
                {"overwrite", true}
            }},
            {"profile", {
                {"name", "contract"},
                {"class", "video"},
                {"container", "mp4"},
                {"alpha_mode", "preserved"},
                {"video", {
                    {"codec", "libx264"},
                    {"pixel_format", "yuv420p"},
                    {"rate_control_mode", "crf"},
                    {"crf", 18}
                }},
                {"audio", {
                    {"mode", "none"},
                    {"codec", ""},
                    {"sample_rate", 48000},
                    {"channels", 2}
                }},
                {"buffering", {
                    {"strategy", "frame_queue"},
                    {"max_frames_in_queue", 2}
                }},
                {"color", {
                    {"space", "rec709"},
                    {"transfer", "bt709"},
                    {"range", "tv"}
                }}
            }}
        }}
    };
    return root.dump();
}

std::string scene_json(std::string blend_mode) {
    nlohmann::json root = {
        {"version", "1.0"},
        {"spec_version", "1.0"},
        {"project", {
            {"id", "proj_01"},
            {"name", "Contract"},
            {"authoring_tool", "codex"}
        }},
        {"compositions", nlohmann::json::array({
            {
                {"id", "main"},
                {"name", "Main"},
                {"width", 64},
                {"height", 64},
                {"duration", 2.0},
                {"frame_rate", {
                    {"numerator", 30},
                    {"denominator", 1}
                }},
                {"layers", nlohmann::json::array({
                    {
                        {"id", "layer_01"},
                        {"type", "solid"},
                        {"name", "Solid"},
                        {"blend_mode", blend_mode},
                        {"opacity", 1.0},
                        {"width", 64},
                        {"height", 64}
                    }
                })}
            }
        })}
    };
    return root.dump();
}

} // namespace

bool run_render_contract_tests() {
    {
        const auto parsed = tachyon::parse_render_job_json(render_job_json("cinematic", true, 8));
        check_true(parsed.value.has_value(), "render job parses with contract fields");
        if (parsed.value.has_value()) {
            const auto validation = tachyon::validate_render_job(*parsed.value);
            check_true(validation.ok(), "render job validates with contract fields");
            check_true(parsed.value->quality_tier == "cinematic", "quality tier parses");
            check_true(parsed.value->motion_blur_enabled, "motion blur enabled parses");
            check_true(parsed.value->motion_blur_samples == 8, "motion blur sample count parses");
            check_true(parsed.value->motion_blur_curve == "box", "motion blur curve parses");
            check_true(parsed.value->working_space == "linear_rec709", "working space parses");
        }
    }

    {
        const auto parsed = tachyon::parse_render_job_json(render_job_json("high", true, 0));
        check_true(parsed.value.has_value(), "motion blur job parses with zero samples");
        if (parsed.value.has_value()) {
            const auto validation = tachyon::validate_render_job(*parsed.value);
            check_true(validation.ok(), "motion blur job validates with zero samples");

            const auto scene_parsed = tachyon::parse_scene_spec_json(scene_json("normal"));
            check_true(scene_parsed.value.has_value(), "motion blur scene parses");
            if (scene_parsed.value.has_value()) {
                const auto plan = tachyon::build_render_plan(*scene_parsed.value, *parsed.value);
                check_true(plan.value.has_value(), "motion blur plan builds with zero samples");
                if (plan.value.has_value()) {
                    check_true(plan.value->motion_blur_samples == 8, "motion blur zero samples defaults to eight");
                }
            }
        }
    }

    {
        const auto parsed = tachyon::parse_scene_spec_json(scene_json("screen"));
        check_true(parsed.value.has_value(), "scene parses with blend mode");
        if (parsed.value.has_value()) {
            const auto validation = tachyon::validate_scene_spec(*parsed.value);
            check_true(validation.ok(), "scene validates with blend mode");

            const auto evaluated = tachyon::scene::evaluate_scene_composition_state(*parsed.value, "main", 0LL);
            check_true(evaluated.has_value(), "scene evaluation resolves composition");
            if (evaluated.has_value()) {
                check_true(!evaluated->layers.empty(), "scene evaluation emits layers");
                if (!evaluated->layers.empty()) {
                    check_true(evaluated->layers.front().blend_mode == "screen", "blend mode propagates into evaluated state");
                }
            }
        }
    }

    {
        const auto parsed = tachyon::parse_scene_spec_json(scene_json("overlay"));
        check_true(parsed.value.has_value(), "scene parses with overlay blend mode");
        if (parsed.value.has_value()) {
            const auto validation = tachyon::validate_scene_spec(*parsed.value);
            check_true(validation.ok(), "scene validates with overlay blend mode");
        }
    }

    {
        const auto parsed = tachyon::parse_scene_spec_json(scene_json("soft_light"));
        check_true(parsed.value.has_value(), "scene parses with soft_light blend mode");
        if (parsed.value.has_value()) {
            const auto validation = tachyon::validate_scene_spec(*parsed.value);
            check_true(validation.ok(), "scene validates with soft_light blend mode");
        }
    }

    {
        const auto parsed = tachyon::parse_render_job_json(render_job_json("high", false, 0));
        check_true(parsed.value.has_value(), "baseline render job parses");
        if (parsed.value.has_value()) {
            const auto scene_parsed = tachyon::parse_scene_spec_json(scene_json("multiply"));
            check_true(scene_parsed.value.has_value(), "baseline scene parses");
            if (scene_parsed.value.has_value()) {
                const auto plan_one = tachyon::build_render_plan(*scene_parsed.value, *parsed.value);
                check_true(plan_one.value.has_value(), "baseline render plan builds");
                if (plan_one.value.has_value()) {
                    check_true(plan_one.value->composition.solid_layer_count == 1, "composition layer counts are summarized");
                    check_true(plan_one.value->composition.precomp_layer_count == 0, "composition precomp count is summarized");
                    const auto exec_one = tachyon::build_render_execution_plan(*plan_one.value, 0);
                    check_true(exec_one.value.has_value(), "baseline execution plan builds");
                    if (exec_one.value.has_value() && !exec_one.value->frame_tasks.empty()) {
                        check_true(exec_one.value->steps.size() >= 6, "render graph exposes ROI/cache preparation steps");
                        const auto key_one = exec_one.value->frame_tasks.front().cache_key.value;

                        tachyon::RenderJob modified_job = *parsed.value;
                        modified_job.quality_tier = "cinematic";
                        modified_job.motion_blur_enabled = true;
                        modified_job.motion_blur_samples = 8;

                        const auto plan_two = tachyon::build_render_plan(*scene_parsed.value, modified_job);
                        check_true(plan_two.value.has_value(), "modified render plan builds");
                        if (plan_two.value.has_value()) {
                            const auto exec_two = tachyon::build_render_execution_plan(*plan_two.value, 0);
                            check_true(exec_two.value.has_value(), "modified execution plan builds");
                            if (exec_two.value.has_value() && !exec_two.value->frame_tasks.empty()) {
                                const auto key_two = exec_two.value->frame_tasks.front().cache_key.value;
                                check_true(key_one != key_two, "quality tier and motion blur affect cache identity");
                            }
                        }
                    }
                }
            }
        }
    }

    return g_failures == 0;
}
