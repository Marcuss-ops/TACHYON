#include <gtest/gtest.h>
#include "tachyon/core/scene/evaluation/evaluator.h"
#include "tachyon/core/spec/schema/objects/scene_spec_core.h"
#include "tachyon/core/scene/evaluator/templates.h"

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

    tachyon::scene::EvaluationVariables eval_vars;
    eval_vars.numeric = &variables;

    const auto evaluated = tachyon::scene::evaluate_composition_state(
        parsed.value->compositions.front(),
        0.0,
        nullptr,
        eval_vars
    );

    EXPECT_EQ(evaluated.layers.front().text_content, "Hello Intro / 0.750000");
}

TEST(TemplateFormatter, NumberPadding) {
    std::unordered_map<std::string, double> vars = {{"score", 42.0}};

    std::string result = tachyon::scene::resolve_template("{{score:000}}", nullptr, &vars);
    EXPECT_EQ(result, "042");

    result = tachyon::scene::resolve_template("{{score:00000}}", nullptr, &vars);
    EXPECT_EQ(result, "00042");

    result = tachyon::scene::resolve_template("{{score:####}}", nullptr, &vars);
    EXPECT_EQ(result, "  42");
}

TEST(TemplateFormatter, DecimalPlaces) {
    std::unordered_map<std::string, double> vars = {{"score", 3.14159}};

    std::string result = tachyon::scene::resolve_template("{{score:.2f}}", nullptr, &vars);
    EXPECT_EQ(result, "3.14");

    result = tachyon::scene::resolve_template("{{score:.4f}}", nullptr, &vars);
    EXPECT_EQ(result, "3.1416");
}

TEST(TemplateFormatter, ThousandSeparators) {
    std::unordered_map<std::string, double> vars = {{"amount", 1234567.89}};

    std::string result = tachyon::scene::resolve_template("{{amount:,.2f}}", nullptr, &vars);
    EXPECT_EQ(result, "1,234,567.89");

    result = tachyon::scene::resolve_template("{{amount:,.0f}}", nullptr, &vars);
    EXPECT_EQ(result, "1,234,568");
}

TEST(TemplateFormatter, DateFormat) {
    std::unordered_map<std::string, std::string> vars = {{"today", "2026-04-27"}};

    std::string result = tachyon::scene::resolve_template("{{today:%d/%m/%Y}}", &vars, nullptr);
    EXPECT_EQ(result, "27/04/2026");

    result = tachyon::scene::resolve_template("{{today:%Y-%m-%d}}", &vars, nullptr);
    EXPECT_EQ(result, "2026-04-27");
}

TEST(TemplateFormatter, FallbackWhenMissing) {
    std::unordered_map<std::string, double> vars = {{"score", 100.0}};

    std::string result = tachyon::scene::resolve_template("Score: {{score}}, Level: {{missing}}", nullptr, &vars);
    EXPECT_EQ(result, "Score: 100, Level: {{missing}}");
}

TEST(TemplateFormatter, StandardSceneVariables) {
    tachyon::scene::EvaluationVariables eval_vars;
    eval_vars.include_standard_vars = false;

    auto numeric_vars = tachyon::scene::make_standard_numeric_vars(30, 1.0, 30.0);
    auto string_vars = tachyon::scene::make_standard_string_vars();

    eval_vars.numeric = &numeric_vars;
    eval_vars.strings = &string_vars;

    std::string result = tachyon::scene::resolve_template("Frame: {{frame}}, Time: {{time}}, FPS: {{fps}}", &string_vars, &numeric_vars);
    EXPECT_TRUE(result.find("Frame: 30") != std::string::npos);
    EXPECT_TRUE(result.find("Time: 1") != std::string::npos);
    EXPECT_TRUE(result.find("FPS: 30") != std::string::npos);
}

TEST(TemplateFormatter, FormatNumberFunction) {
    EXPECT_EQ(tachyon::scene::format_number(42.0, "000"), "042");
    EXPECT_EQ(tachyon::scene::format_number(3.14159, ".2f"), "3.14");
    EXPECT_EQ(tachyon::scene::format_number(1234567.89, ",.2f"), "1,234,567.89");
    EXPECT_EQ(tachyon::scene::format_number(100.0, ""), "100");
}

TEST(TemplateFormatter, LocaleAwareFormatting) {
    // Test Italian locale (dot for thousands, comma for decimal)
    std::string result = tachyon::scene::format_number(1234567.89, ",.2f", "it-IT");
    EXPECT_EQ(result, "1.234.567,89");
    
    // Test with padding in different locale
    result = tachyon::scene::format_number(42.0, "000", "it-IT");
    EXPECT_EQ(result, "042");
}
