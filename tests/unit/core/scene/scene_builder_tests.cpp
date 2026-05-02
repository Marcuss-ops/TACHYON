#include "tachyon/scene/builder.h"
#include "tachyon/core/spec/schema/objects/scene_spec_core.h"
#include "tachyon/core/spec/scene_spec_serialize.h"
#include <gtest/gtest.h>

namespace tachyon::scene {

TEST(SceneBuilder, BasicComposition) {
    auto scene = Composition("main")
        .size(1920, 1080)
        .fps(60)
        .duration(10.0)
        .background(BackgroundSpec::from_string("#ff0000"))
        .layer("bg", [](LayerBuilder& l) {
            l.type("solid").color(ColorSpec(255, 0, 0, 255));
        })
        .build_scene();

    ASSERT_EQ(scene.compositions.size(), 1);
    const auto& comp = scene.compositions[0];
    EXPECT_EQ(comp.id, "main");
    EXPECT_EQ(comp.width, 1920);
    EXPECT_EQ(comp.height, 1080);
    EXPECT_EQ(comp.duration, 10.0);
    EXPECT_EQ(comp.frame_rate.numerator, 60);
    ASSERT_TRUE(comp.background.has_value());
    EXPECT_EQ(comp.background->type, BackgroundType::Color);
    EXPECT_EQ(comp.background->value, "#ff0000");

    ASSERT_EQ(comp.layers.size(), 1);
    EXPECT_EQ(comp.layers[0].id, "bg");
    EXPECT_EQ(comp.layers[0].type, "solid");
}

TEST(SceneBuilder, AnimatedProperties) {
    auto comp = Composition("main")
        .layer("title", [](LayerBuilder& l) {
            l.type("text")
             .text("Hello")
             .opacity(anim::lerp(0, 1, 1.0));
        })
        .build();

    ASSERT_EQ(comp.layers.size(), 1);
    const auto& layer = comp.layers[0];
    EXPECT_EQ(layer.opacity_property.keyframes.size(), 2);
    EXPECT_EQ(layer.opacity_property.keyframes[0].time, 0.0);
    EXPECT_EQ(layer.opacity_property.keyframes[0].value, 0.0);
    EXPECT_EQ(layer.opacity_property.keyframes[1].time, 1.0);
    EXPECT_EQ(layer.opacity_property.keyframes[1].value, 1.0);
}

TEST(SceneBuilder, Transitions) {
    auto comp = Composition("main")
        .layer("text", [](LayerBuilder& l) {
            l.enter().id("fade").duration(0.5).done()
             .exit().id("slide").delay(1.0).done();
        })
        .build();

    ASSERT_EQ(comp.layers.size(), 1);
    const auto& layer = comp.layers[0];
    EXPECT_EQ(layer.transition_in.transition_id, "fade");
    EXPECT_EQ(layer.transition_in.duration, 0.5);
    EXPECT_EQ(layer.transition_out.transition_id, "slide");
    EXPECT_EQ(layer.transition_out.delay, 1.0);
}

TEST(SceneBuilder, MaterialBuilder) {
    auto comp = Composition("main")
        .layer("mesh", [](LayerBuilder& l) {
            l.type("mesh");
            l.material()
                .base_color(ColorSpec(10, 20, 30, 255))
                .metallic(0.8)
                .roughness(0.25)
                .done();
        })
        .build();

    ASSERT_EQ(comp.layers.size(), 1);
    const auto& layer = comp.layers[0];
    ASSERT_TRUE(layer.fill_color.value.has_value());
    EXPECT_EQ(*layer.fill_color.value, ColorSpec(10, 20, 30, 255));
    ASSERT_TRUE(layer.metallic.value.has_value());
    ASSERT_TRUE(layer.roughness.value.has_value());
    EXPECT_EQ(*layer.metallic.value, 0.8);
    EXPECT_EQ(*layer.roughness.value, 0.25);
}

TEST(SceneBuilder, Common3DMethodsWorkOnAnyLayer) {
    auto comp = Composition("main")
        .layer("title", [](LayerBuilder& l) {
            l.type("text")
             .text("Hello 3D")
             .position3d(10.0, 20.0, 30.0)
             .rotation3d(0.0, 45.0, 0.0)
             .scale3d(1.2, 1.2, 1.2)
             .pivot3d(5.0, 6.0, 7.0)
             .extrude3d(0.25)
             .bevel3d(0.03);
        })
        .build();

    ASSERT_EQ(comp.layers.size(), 1);
    const auto& layer = comp.layers[0];
    EXPECT_TRUE(layer.is_3d);
    ASSERT_TRUE(layer.transform3d.position_property.value.has_value());
    ASSERT_TRUE(layer.transform3d.rotation_property.value.has_value());
    ASSERT_TRUE(layer.transform3d.scale_property.value.has_value());
    ASSERT_TRUE(layer.transform3d.anchor_point_property.value.has_value());
    EXPECT_EQ(layer.transform3d.position_property.value->x, 10.0f);
    EXPECT_EQ(layer.transform3d.position_property.value->y, 20.0f);
    EXPECT_EQ(layer.transform3d.position_property.value->z, 30.0f);
    EXPECT_EQ(layer.transform3d.rotation_property.value->y, 45.0f);
    EXPECT_EQ(layer.transform3d.scale_property.value->x, 1.2f);
    EXPECT_EQ(layer.transform3d.anchor_point_property.value->z, 7.0f);
    EXPECT_EQ(layer.extrusion_depth, 0.25);
    EXPECT_EQ(layer.bevel_size, 0.03);
}

TEST(SceneBuilder, RoundTrip) {
    SceneSpec via_builder = Composition("main")
        .size(1920, 1080).fps(60).duration(5.0)
        .layer("bg", [](LayerBuilder& l) {
            l.type("procedural");
        })
        .build_scene();

    std::string json_text = R"({
        "schema_version": "1.0.0",
        "project": { "id": "", "name": "", "authoring_tool": "" },
        "compositions": [
            {
                "id": "main",
                "name": "",
                "width": 1920,
                "height": 1080,
                "duration": 5.0,
                "fps": 60,
                "layers": [
                    {
                        "id": "bg",
                        "name": "",
                        "type": "procedural"
                    }
                ]
            }
        ]
    })";

    auto via_json_res = tachyon::parse_scene_spec_json(json_text);
    ASSERT_TRUE(via_json_res.value.has_value());
    const auto& via_json = *via_json_res.value;

    EXPECT_EQ(serialize_scene_spec(via_builder), serialize_scene_spec(via_json));
}

} // namespace tachyon::scene
