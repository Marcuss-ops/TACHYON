#include "tachyon/scene/builder.h"
#include <iostream>

using namespace tachyon;
using namespace tachyon::scene;

#ifdef _WIN32
#define EXPORT extern "C" __declspec(dllexport)
#else
#define EXPORT extern "C"
#endif

EXPORT void build_scene(SceneSpec& scene) {
    auto comp_builder = Composition("png_tilt_3d")
        .size(1920, 1080)
        .duration(2.0)
        .fps(60)
        .clear({15, 17, 22, 255})
        .camera3d_layer("cam", [](LayerBuilder& l) {
            l.position3d(0, 0, -600)
             .camera_poi(0, 0, 0)
             .camera_zoom(45);
        })
        .light_layer("ambient", [](LayerBuilder& l) {
            l.light_type("ambient")
             .light_intensity(0.6);
        })
        .light_layer("point", [](LayerBuilder& l) {
            l.light_type("point")
             .position3d(250, -200, -300)
             .light_color({255, 250, 240, 255})
             .light_intensity(1.5);
        })
        .layer("logo_3d", [](LayerBuilder& l) {
            l.image("assets/test/logo_alpha.png")
             .size(512, 512)
             .position3d(0, 0, 0)
             .rotation3d(0, 25, 0); // Static tilt to verify 3D
        });
        
    scene = comp_builder.build_scene();
}
