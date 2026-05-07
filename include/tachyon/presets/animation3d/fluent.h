#pragma once

#include "tachyon/scene/builder.h"

namespace tachyon::presets::animation3d {

inline scene::LayerBuilder& tilt(
    scene::LayerBuilder& layer,
    double tilt_x = 0.0,
    double tilt_y = 0.0,
    double smoothness = 0.5) {

    layer.transform3d().modifier(
        "tachyon.modifier3d.tilt",
        {
            {"tilt_x", tilt_x},
            {"tilt_y", tilt_y},
            {"smoothness", smoothness},
        }).done();
    return layer;
}

inline scene::LayerBuilder& parallax(
    scene::LayerBuilder& layer,
    double depth = 0.0) {

    layer.transform3d().modifier(
        "tachyon.modifier3d.parallax",
        {
            {"depth", depth},
        }).done();
    return layer;
}

inline scene::LayerBuilder& orbit_y(
    scene::LayerBuilder& layer,
    double duration = 1.0,
    double delay = 0.0,
    double intensity = 1.0) {

    layer.transform3d().animation_preset("tachyon.anim3d.orbit_y", duration, delay, intensity).done();
    return layer;
}

inline scene::LayerBuilder& camera_push_in(
    scene::LayerBuilder& layer,
    double duration = 1.0,
    double delay = 0.0,
    double intensity = 1.0) {

    layer.transform3d().animation_preset("tachyon.anim3d.camera_push_in", duration, delay, intensity).done();
    return layer;
}

inline scene::LayerBuilder& turntable(
    scene::LayerBuilder& layer,
    double duration = 1.0,
    double delay = 0.0,
    double intensity = 1.0) {

    layer.transform3d().animation_preset("tachyon.anim3d.turntable", duration, delay, intensity).done();
    return layer;
}

inline scene::LayerBuilder& float_z(
    scene::LayerBuilder& layer,
    double duration = 1.0,
    double delay = 0.0,
    double intensity = 1.0) {

    layer.transform3d().animation_preset("tachyon.anim3d.float_z", duration, delay, intensity).done();
    return layer;
}

} // namespace tachyon::presets::animation3d
