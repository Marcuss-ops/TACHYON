#include "tachyon/render_job.h"
#include "tachyon/scene_spec.h"

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

} // namespace

bool run_scene_spec_tests() {
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
                        {
                            "id": "title",
                            "type": "text",
                            "name": "Title",
                            "start_time": 0,
                            "in_point": 0,
                            "out_point": 120,
                            "opacity": 1
                        }
                    ]
                }
            ]
        })";

        const auto parsed = tachyon::parse_scene_spec_json(text);
        check_true(parsed.value.has_value(), "valid scene spec should parse");
        if (parsed.value.has_value()) {
            const auto validation = tachyon::validate_scene_spec(*parsed.value);
            check_true(validation.ok(), "valid scene spec should validate");
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

    return g_failures == 0;
}

bool run_render_job_tests() {
    const std::string text = R"({
        "job_id": "job_001",
        "scene_ref": "scene.json",
        "composition_target": "main",
        "frame_range": { "start": 0, "end": 120 },
        "output": {
            "destination": { "path": "out/intro.mp4", "overwrite": false },
            "profile": {
                "name": "delivery_h264_mp4",
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
                    "codec": "aac",
                    "sample_rate": 48000,
                    "channels": 2
                },
                "buffering": {
                    "strategy": "pipe",
                    "max_frames_in_queue": 8
                },
                "color": {
                    "transfer": "bt709",
                    "range": "tv"
                }
            }
        }
    })";

    const auto parsed = tachyon::parse_render_job_json(text);
    check_true(parsed.value.has_value(), "valid render job should parse");
    if (parsed.value.has_value()) {
        const auto validation = tachyon::validate_render_job(*parsed.value);
        check_true(validation.ok(), "valid render job should validate");
    }

    return g_failures == 0;
}
