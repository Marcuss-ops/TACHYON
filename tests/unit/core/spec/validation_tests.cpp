#include "tachyon/core/spec/schema/objects/scene_spec.h"
#include "tachyon/core/spec/schema/objects/scene_spec_core.h"
#include <iostream>

namespace {
int g_failures = 0;
void check_true(bool condition, const std::string& message) {
    if (!condition) {
        ++g_failures;
        std::cerr << "FAIL: " << message << '\n';
    }
}
}

bool run_scene_spec_validation_tests() {
    g_failures = 0;
    {
        const std::string text = R"({
            "spec_version": "1.0",
            "project": { "id": "proj_001", "name": "Intro" },
            "compositions": [
                {
                    "id": "main",
                    "width": 1920, "height": 1080, "duration": 120, "frame_rate": 30,
                    "layers": [
                        { "id": "title", "type": "text", "name": "Title", "start_time": 0, "in_point": 0, "out_point": 120 },
                        { "id": "title", "type": "text", "name": "Title 2", "start_time": 0, "in_point": 0, "out_point": 120 }
                    ]
                }
            ]
        })";

        const auto parsed = tachyon::parse_scene_spec_json(text);
        if (parsed.value.has_value()) {
            const auto validation = tachyon::validate_scene_spec(*parsed.value);
            check_true(!validation.ok(), "duplicate layer ids should fail validation");
        }
    }

    {
        const std::string text = R"({
            "spec_version": "1.0",
            "project": { "id": "proj_refs", "name": "Reference Scene" },
            "compositions": [
                {
                    "id": "main",
                    "width": 1920, "height": 1080, "duration": 120, "frame_rate": 30,
                    "layers": [
                        { "id": "hero", "type": "image", "name": "hero", "start_time": 0, "in_point": 0, "out_point": 120, "parent": "missing_parent" }
                    ]
                }
            ]
        })";

        const auto parsed = tachyon::parse_scene_spec_json(text);
        if (parsed.value.has_value()) {
            const auto validation = tachyon::validate_scene_spec(*parsed.value);
            check_true(!validation.ok(), "invalid parent reference should fail validation");
        }
    }

    return g_failures == 0;
}
