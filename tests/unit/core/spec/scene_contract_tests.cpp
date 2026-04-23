#include "tachyon/core/spec/schema/objects/scene_spec.h"
#include "tachyon/core/spec/schema/objects/scene_spec_core.h"
#include "tachyon/core/spec/validation/scene_validator.h"
#include "tachyon/core/spec/compilation/scene_compiler.h"
#include <nlohmann/json.hpp>
#include <iostream>
#include <string>

namespace {

using json = nlohmann::json;

int g_failures = 0;

void check_true(bool condition, const std::string& message) {
    if (!condition) {
        ++g_failures;
        std::cerr << "FAIL: " << message << '\n';
    }
}

} // namespace

bool run_scene_contract_tests() {
    g_failures = 0;

    // 1. Roundtrip completo di una SceneSpec con mask_paths (parse -> serialize -> parse)
    {
        const std::string text = R"({
            "spec_version": "1.0",
            "project": { "id": "mask_test", "name": "Mask Test" },
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
                            "id": "layer1",
                            "type": "solid",
                            "name": "Background",
                            "in_point": 0,
                            "out_point": 120,
                            "mask_paths": [
                                {
                                    "is_closed": true,
                                    "is_inverted": false,
                                    "vertices": [
                                        { "position": [0, 0], "in_tangent": [0, 0], "out_tangent": [0, 0] },
                                        { "position": [100, 0], "in_tangent": [0, 0], "out_tangent": [0, 0] },
                                        { "position": [50, 100], "in_tangent": [0, 0], "out_tangent": [0, 0] }
                                    ]
                                }
                            ]
                        }
                    ]
                }
            ]
        })";

        const auto parsed1 = tachyon::parse_scene_spec_json(text);
        check_true(parsed1.value.has_value(), "roundtrip_mask: initial parse succeeds");
        if (!parsed1.value.has_value()) {
            return g_failures == 0;
        }

        const auto& scene1 = *parsed1.value;
        check_true(!scene1.compositions.empty(), "roundtrip_mask: has compositions");
        check_true(!scene1.compositions[0].layers.empty(), "roundtrip_mask: has layers");
        check_true(!scene1.compositions[0].layers[0].mask_paths.empty(), "roundtrip_mask: layer has mask_paths");

        const json serialized = tachyon::serialize_scene_spec(scene1);
        const std::string serialized_text = serialized.dump(2);

        const auto parsed2 = tachyon::parse_scene_spec_json(serialized_text);
        check_true(parsed2.value.has_value(), "roundtrip_mask: re-parse succeeds");
        if (!parsed2.value.has_value()) {
            return g_failures == 0;
        }

        const auto& scene2 = *parsed2.value;
        check_true(scene2.compositions.size() == scene1.compositions.size(), "roundtrip_mask: composition count matches");
        if (!scene2.compositions.empty() && !scene1.compositions.empty()) {
            const auto& l1 = scene1.compositions[0].layers[0];
            const auto& l2 = scene2.compositions[0].layers[0];
            check_true(l1.mask_paths.size() == l2.mask_paths.size(), "roundtrip_mask: mask_paths count matches");
            if (l1.mask_paths.size() == l2.mask_paths.size() && !l1.mask_paths.empty()) {
                check_true(l1.mask_paths[0].is_closed == l2.mask_paths[0].is_closed, "roundtrip_mask: mask_paths is_closed matches");
                check_true(l1.mask_paths[0].is_inverted == l2.mask_paths[0].is_inverted, "roundtrip_mask: mask_paths is_inverted matches");
                check_true(l1.mask_paths[0].vertices.size() == l2.mask_paths[0].vertices.size(), "roundtrip_mask: vertices count matches");
            }
        }
    }

    // 2. Validazione di mask_paths con vertici insufficienti (< 2) deve generare warning
    {
        const std::string text = R"({
            "spec_version": "1.0",
            "project": { "id": "mask_warn", "name": "Mask Warning" },
            "compositions": [
                {
                    "id": "main",
                    "width": 1920,
                    "height": 1080,
                    "duration": 120,
                    "frame_rate": 30,
                    "layers": [
                        {
                            "id": "layer1",
                            "type": "solid",
                            "name": "Background",
                            "in_point": 0,
                            "out_point": 120,
                            "mask_paths": [
                                {
                                    "is_closed": true,
                                    "vertices": [
                                        { "position": [0, 0] }
                                    ]
                                }
                            ]
                        }
                    ]
                }
            ]
        })";

        const auto parsed = tachyon::parse_scene_spec_json(text);
        check_true(parsed.value.has_value(), "mask_warn: parse succeeds");
        if (parsed.value.has_value()) {
            tachyon::core::SceneValidator validator;
            const auto result = validator.validate(*parsed.value);
            check_true(!result.is_valid() || result.warning_count > 0, "mask_warn: insufficient vertices should generate warning or fail");
        }
    }

    // 3. Validazione di parent mancante deve fallire
    {
        const std::string text = R"({
            "spec_version": "1.0",
            "project": { "id": "parent_test", "name": "Parent Test" },
            "compositions": [
                {
                    "id": "main",
                    "width": 1920,
                    "height": 1080,
                    "duration": 120,
                    "frame_rate": 30,
                    "layers": [
                        {
                            "id": "layer1",
                            "type": "solid",
                            "name": "Child",
                            "in_point": 0,
                            "out_point": 120,
                            "parent": "nonexistent_parent"
                        }
                    ]
                }
            ]
        })";

        const auto parsed = tachyon::parse_scene_spec_json(text);
        check_true(parsed.value.has_value(), "parent_missing: parse succeeds");
        if (parsed.value.has_value()) {
            const auto validation = tachyon::validate_scene_spec(*parsed.value);
            check_true(!validation.ok(), "parent_missing: missing parent reference should fail validation");
        }
    }

    // 4. Validazione di in_point > out_point deve fallire
    {
        const std::string text = R"({
            "spec_version": "1.0",
            "project": { "id": "timing_test", "name": "Timing Test" },
            "compositions": [
                {
                    "id": "main",
                    "width": 1920,
                    "height": 1080,
                    "duration": 120,
                    "frame_rate": 30,
                    "layers": [
                        {
                            "id": "layer1",
                            "type": "solid",
                            "name": "Bad Timing",
                            "in_point": 100,
                            "out_point": 50
                        }
                    ]
                }
            ]
        })";

        const auto parsed = tachyon::parse_scene_spec_json(text);
        check_true(parsed.value.has_value(), "in_out: parse succeeds");
        if (parsed.value.has_value()) {
            tachyon::core::SceneValidator validator;
            const auto result = validator.validate(*parsed.value);
            check_true(!result.is_valid(), "in_out: in_point > out_point should fail validation");
        }
    }

    // 5. Validazione di duplicate layer IDs deve fallire
    {
        const std::string text = R"({
            "spec_version": "1.0",
            "project": { "id": "dup_test", "name": "Duplicate ID Test" },
            "compositions": [
                {
                    "id": "main",
                    "width": 1920,
                    "height": 1080,
                    "duration": 120,
                    "frame_rate": 30,
                    "layers": [
                        {
                            "id": "layer1",
                            "type": "solid",
                            "name": "First",
                            "in_point": 0,
                            "out_point": 120
                        },
                        {
                            "id": "layer1",
                            "type": "solid",
                            "name": "Second",
                            "in_point": 0,
                            "out_point": 120
                        }
                    ]
                }
            ]
        })";

        const auto parsed = tachyon::parse_scene_spec_json(text);
        check_true(parsed.value.has_value(), "duplicate_id: parse succeeds");
        if (parsed.value.has_value()) {
            const auto validation = tachyon::validate_scene_spec(*parsed.value);
            check_true(!validation.ok(), "duplicate_id: duplicate layer IDs should fail validation");
        }
    }

    // 6. Cache identity: due scene identiche (con mask_paths) devono avere lo stesso hash
    {
        const std::string text = R"({
            "spec_version": "1.0",
            "project": { "id": "hash_test", "name": "Hash Test" },
            "compositions": [
                {
                    "id": "main",
                    "width": 1920,
                    "height": 1080,
                    "duration": 120,
                    "frame_rate": 30,
                    "layers": [
                        {
                            "id": "layer1",
                            "type": "solid",
                            "name": "Background",
                            "in_point": 0,
                            "out_point": 120,
                            "mask_paths": [
                                {
                                    "is_closed": true,
                                    "vertices": [
                                        { "position": [0, 0], "in_tangent": [0, 0], "out_tangent": [0, 0] },
                                        { "position": [100, 0], "in_tangent": [0, 0], "out_tangent": [0, 0] },
                                        { "position": [50, 100], "in_tangent": [0, 0], "out_tangent": [0, 0] }
                                    ]
                                }
                            ]
                        }
                    ]
                }
            ]
        })";

        const auto parsed1 = tachyon::parse_scene_spec_json(text);
        const auto parsed2 = tachyon::parse_scene_spec_json(text);
        check_true(parsed1.value.has_value() && parsed2.value.has_value(), "cache_identity: both parses succeed");
        if (parsed1.value.has_value() && parsed2.value.has_value()) {
            tachyon::SceneCompiler compiler;
            const auto compiled1 = compiler.compile(*parsed1.value);
            const auto compiled2 = compiler.compile(*parsed2.value);
            check_true(compiled1.value.has_value() && compiled2.value.has_value(), "cache_identity: both compiles succeed");
            if (compiled1.value.has_value() && compiled2.value.has_value()) {
                check_true(compiled1.value->scene_hash == compiled2.value->scene_hash, "cache_identity: identical scenes have same hash");
            }
        }
    }

    // 7. Cache identity: due scene con mask_paths diversi devono avere hash diverso
    {
        const std::string text_a = R"({
            "spec_version": "1.0",
            "project": { "id": "hash_diff", "name": "Hash Diff" },
            "compositions": [
                {
                    "id": "main",
                    "width": 1920,
                    "height": 1080,
                    "duration": 120,
                    "frame_rate": 30,
                    "layers": [
                        {
                            "id": "layer1",
                            "type": "solid",
                            "name": "Background",
                            "in_point": 0,
                            "out_point": 120,
                            "mask_paths": [
                                {
                                    "is_closed": true,
                                    "vertices": [
                                        { "position": [0, 0], "in_tangent": [0, 0], "out_tangent": [0, 0] },
                                        { "position": [100, 0], "in_tangent": [0, 0], "out_tangent": [0, 0] }
                                    ]
                                }
                            ]
                        }
                    ]
                }
            ]
        })";

        const std::string text_b = R"({
            "spec_version": "1.0",
            "project": { "id": "hash_diff", "name": "Hash Diff" },
            "compositions": [
                {
                    "id": "main",
                    "width": 1920,
                    "height": 1080,
                    "duration": 120,
                    "frame_rate": 30,
                    "layers": [
                        {
                            "id": "layer1",
                            "type": "solid",
                            "name": "Background",
                            "in_point": 0,
                            "out_point": 120,
                            "mask_paths": [
                                {
                                    "is_closed": true,
                                    "vertices": [
                                        { "position": [0, 0], "in_tangent": [0, 0], "out_tangent": [0, 0] },
                                        { "position": [200, 0], "in_tangent": [0, 0], "out_tangent": [0, 0] }
                                    ]
                                }
                            ]
                        }
                    ]
                }
            ]
        })";

        const auto parsed_a = tachyon::parse_scene_spec_json(text_a);
        const auto parsed_b = tachyon::parse_scene_spec_json(text_b);
        check_true(parsed_a.value.has_value() && parsed_b.value.has_value(), "cache_diff: both parses succeed");
        if (parsed_a.value.has_value() && parsed_b.value.has_value()) {
            tachyon::SceneCompiler compiler;
            const auto compiled_a = compiler.compile(*parsed_a.value);
            const auto compiled_b = compiler.compile(*parsed_b.value);
            check_true(compiled_a.value.has_value() && compiled_b.value.has_value(), "cache_diff: both compiles succeed");
            if (compiled_a.value.has_value() && compiled_b.value.has_value()) {
                check_true(compiled_a.value->scene_hash != compiled_b.value->scene_hash, "cache_diff: different mask_paths have different hash");
            }
        }
    }

    return g_failures == 0;
}
