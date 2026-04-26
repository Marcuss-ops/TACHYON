#include <gtest/gtest.h>
#include "tachyon/core/scene/evaluation/evaluator.h"
#include "tachyon/core/spec/schema/objects/scene_spec_core.h"

#include <filesystem>
#include <fstream>
#include <cmath>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>

namespace {

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

} // namespace

using namespace tachyon;

TEST(SceneEvaluator, MinimalSceneGraph) {
    const auto path = tests_root() / "fixtures" / "scenes" / "scene_graph_minimal.json";
    const auto text = read_text_file(path);
    ASSERT_FALSE(text.empty()) << "scene graph fixture should be readable";

    const auto parsed = tachyon::parse_scene_spec_json(text);
    ASSERT_TRUE(parsed.value.has_value()) << "scene graph fixture should parse";
    
    const auto validation = tachyon::validate_scene_spec(*parsed.value);
    EXPECT_TRUE(validation.ok()) << "scene graph fixture should validate";

    const auto evaluated = tachyon::scene::evaluate_composition_state(parsed.value->compositions.front(), std::int64_t(30));
    ASSERT_EQ(evaluated.layers.size(), 2) << "scene graph should preserve layer order";
    
    EXPECT_EQ(evaluated.layers[0].id, "parent");
    EXPECT_EQ(evaluated.layers[1].id, "child");
    EXPECT_EQ(evaluated.layers[0].layer_index, 0);
    EXPECT_EQ(evaluated.layers[1].layer_index, 1);
    
    EXPECT_NEAR(evaluated.layers[0].opacity, 0.5, 1e-6);
    EXPECT_NEAR(evaluated.layers[1].opacity, 0.75, 1e-6);
    
    const auto child_wp = evaluated.layers[1].world_position3;
    EXPECT_NEAR(child_wp.x, 120.0, 1e-6);
    EXPECT_NEAR(child_wp.y, 60.0, 1e-6);
    
    EXPECT_TRUE(evaluated.layers[0].visible);
    EXPECT_TRUE(evaluated.layers[1].visible);
}

TEST(SceneEvaluator, KeyframeAnimation) {
    tachyon::LayerSpec layer;
    layer.id = "title";
    layer.type = "text";
    layer.name = "Title";
    layer.start_time = 1.0;
    layer.in_point = 0.0;
    layer.out_point = 10.0;
    layer.opacity_property.keyframes = {
        {0.0, 0.0, animation::InterpolationMode::Linear, animation::EasingPreset::EaseIn},
        {2.0, 1.0}
    };
    layer.transform.position_property.keyframes = {
        {0.0, {0.0f, 0.0f}},
        {2.0, {100.0f, 50.0f}}
    };
    layer.transform.rotation_property.keyframes = {
        {0.0, 0.0},
        {2.0, 90.0}
    };
    layer.transform.scale_property.value = tachyon::math::Vector2{1.0f, 1.0f};

    CompositionSpec comp;
    comp.id = "main";
    comp.layers.push_back(layer);

    {
        // Sample at t=0.5 (relative to layer start) -> absolute 1.5s
        const auto evaluated = tachyon::scene::evaluate_composition_state(comp, 1.5);
        const auto& el = evaluated.layers.front();
        
        // Eased opacity at 0.5/2.0 = 0.25 normalized time
        EXPECT_LT(el.opacity, 0.25) << "EaseIn should result in lower value than linear at start";
        
        // Linear position: 0.25 * 100 = 25, 0.25 * 50 = 12.5
        EXPECT_NEAR(el.local_transform.position.x, 25.0f, 1e-5f);
        EXPECT_NEAR(el.local_transform.position.y, 12.5f, 1e-5f);
        
        // Linear rotation: 0.25 * 90 = 22.5
        EXPECT_NEAR(el.local_transform.rotation_rad, (22.5 * 3.1415926535 / 180.0), 1e-5);
    }
}

TEST(SceneEvaluator, TemplateResolver) {
    const auto path = tests_root() / "fixtures" / "scenes" / "template_scene.json";
    const auto text = read_text_file(path);
    ASSERT_FALSE(text.empty());

    const auto parsed = tachyon::parse_scene_spec_json(text);
    ASSERT_TRUE(parsed.value.has_value());

    std::unordered_map<std::string, double> variables;
    variables["user_id"] = 123.0;
    variables["score"] = 0.75;
    variables["config.speed"] = 2.0;

    const auto evaluated = tachyon::scene::evaluate_composition_state(
        parsed.value->compositions.front(),
        0.0,
        nullptr,
        &variables
    );
    
    EXPECT_EQ(evaluated.layers.front().text_content, "Hello Intro / 0.750000");
}
