#include "tachyon/scene/builder.h"
#include "tachyon/core/spec/schema/objects/background_spec.h"

#include <iostream>

using namespace tachyon;
using namespace tachyon::scene;

int main() {
    auto scene = SceneBuilder()
        .project("test_13", "3D Showcase")
        .composition("main", [](CompositionBuilder& comp) {
            comp.size(1280, 720)
                .duration(1.0)
                .fps(30)
                .background(BackgroundSpec::from_string("#06070b"));

            comp.camera3d_layer("cam", [](LayerBuilder& l) {
                l.position3d(640, 360, -1650)
                    .camera_poi(640, 360, 0)
                    .camera_type("two_node");
            });

            comp.light_layer("key_light", [](LayerBuilder& l) {
                l.position3d(390, 180, -900)
                    .light_type("point")
                    .light_color(ColorSpec(255, 235, 210, 255))
                    .light_intensity(7800000)
                    .casts_shadows(true)
                    .shadow_radius(28);
            });

            comp.light_layer("fill_light", [](LayerBuilder& l) {
                l.position3d(980, 540, -650)
                    .light_type("point")
                    .light_color(ColorSpec(130, 180, 255, 255))
                    .light_intensity(2500000)
                    .casts_shadows(false);
            });

            comp.light_layer("rim_light", [](LayerBuilder& l) {
                l.position3d(1020, 140, -1350)
                    .light_type("point")
                    .light_color(ColorSpec(180, 120, 255, 255))
                    .light_intensity(1800000)
                    .casts_shadows(false);
            });

            comp.mesh_layer("floor", [](LayerBuilder& l) {
                l.size(5200, 5200)
                    .position3d(640, 540, 220)
                    .rotation3d(90, 0, 0)
                    .color(ColorSpec(85, 88, 96, 255))
                    .material()
                    .base_color(ColorSpec(85, 88, 96, 255))
                    .metallic(0.15)
                    .roughness(0.9)
                    .done();
            });

            comp.mesh_layer("back_plate", [](LayerBuilder& l) {
                l.size(760, 420)
                    .position3d(640, 360, -420)
                    .color(ColorSpec(32, 38, 56, 255))
                    .material()
                    .base_color(ColorSpec(32, 38, 56, 255))
                    .metallic(0.0)
                    .roughness(0.7)
                    .done()
                    .emission_strength(0.15);
            });

            comp.mesh_layer("glass_card", [](LayerBuilder& l) {
                l.size(360, 360)
                    .position3d(540, 360, -160)
                    .rotation3d(12, -24, 6)
                    .color(ColorSpec(230, 236, 248, 255))
                    .material()
                    .base_color(ColorSpec(230, 236, 248, 255))
                    .metallic(0.0)
                    .roughness(0.08)
                    .done()
                    .transmission(0.65)
                    .ior(1.45)
                    .emission_strength(0.05);
            });

            comp.mesh_layer("metal_card", [](LayerBuilder& l) {
                l.size(260, 260)
                    .position3d(825, 390, -260)
                    .rotation3d(-8, 28, -4)
                    .color(ColorSpec(255, 156, 64, 255))
                    .material()
                    .base_color(ColorSpec(255, 156, 64, 255))
                    .metallic(1.0)
                    .roughness(0.16)
                    .done()
                    .emission_strength(0.25);
            });

            comp.mesh_layer("accent_card", [](LayerBuilder& l) {
                l.size(180, 180)
                    .position3d(705, 265, -90)
                    .rotation3d(0, 45, 0)
                    .color(ColorSpec(90, 210, 255, 255))
                    .material()
                    .base_color(ColorSpec(90, 210, 255, 255))
                    .metallic(0.0)
                    .roughness(0.3)
                    .done()
                    .emission_strength(1.2);
            });
        })
        .build();

    std::cout << "Built C++ 3D showcase scene: " << scene.project.name << '\n';
    std::cout << "Compositions: " << scene.compositions.size() << '\n';
    return 0;
}
