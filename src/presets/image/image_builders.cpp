#include "tachyon/presets/image/image_builders.h"
#include "tachyon/presets/transition/transition_builders.h"
#include "tachyon/core/animation/easing.h"

namespace tachyon::presets {

namespace {

LayerSpec make_image_layer_base(const ImageParams& p) {
    LayerSpec l;
    l.id          = "image_layer";
    l.name        = "Image";
    l.type        = "image";
    l.kind        = LayerType::Image;
    l.enabled     = true;
    l.visible     = true;
    l.start_time  = p.in_point;
    l.in_point    = p.in_point;
    l.out_point   = p.out_point;
    l.width       = static_cast<int>(p.w);
    l.height      = static_cast<int>(p.h);
    l.opacity     = static_cast<double>(p.opacity);
    l.asset_id    = p.path; // We use the path as the asset ID reference
    
    l.transform.position_x = static_cast<double>(p.x);
    l.transform.position_y = static_cast<double>(p.y);
    l.transform.anchor_point.value = { p.w * 0.5f, p.h * 0.5f };

    if (!p.enter_preset.empty()) {
        TransitionParams tp;
        tp.id = p.enter_preset;
        tp.duration = p.enter_duration;
        l.transition_in = build_transition_enter(tp);
    }
    if (!p.exit_preset.empty()) {
        TransitionParams tp;
        tp.id = p.exit_preset;
        tp.duration = p.exit_duration;
        l.transition_out = build_transition_exit(tp);
    }

    return l;
}

} // namespace

LayerSpec build_image_static(const ImageParams& p) {
    return make_image_layer_base(p);
}

LayerSpec build_image_slow_zoom(const ImageParams& p) {
    auto l = make_image_layer_base(p);
    l.transform.scale_property.value = { p.zoom_from * 100.0f, p.zoom_from * 100.0f };
    l.transform.scale_property.keyframes = {
        { static_cast<float>(p.in_point),  { p.zoom_from * 100.0f, p.zoom_from * 100.0f } },
        { static_cast<float>(p.out_point), { p.zoom_to * 100.0f,   p.zoom_to * 100.0f } }
    };
    return l;
}

LayerSpec build_image_pan_left(const ImageParams& p) {
    auto l = make_image_layer_base(p);
    float dist = p.w * p.pan_distance;
    l.transform.position_property.value = { p.x + dist, p.y };
    l.transform.position_property.keyframes = {
        { static_cast<float>(p.in_point),  { p.x + dist, p.y } },
        { static_cast<float>(p.out_point), { p.x - dist, p.y } }
    };
    return l;
}

LayerSpec build_image_pan_right(const ImageParams& p) {
    auto l = make_image_layer_base(p);
    float dist = p.w * p.pan_distance;
    l.transform.position_property.value = { p.x - dist, p.y };
    l.transform.position_property.keyframes = {
        { static_cast<float>(p.in_point),  { p.x - dist, p.y } },
        { static_cast<float>(p.out_point), { p.x + dist, p.y } }
    };
    return l;
}

LayerSpec build_image_pan_up(const ImageParams& p) {
    auto l = make_image_layer_base(p);
    float dist = p.h * p.pan_distance;
    l.transform.position_property.value = { p.x, p.y + dist };
    l.transform.position_property.keyframes = {
        { static_cast<float>(p.in_point),  { p.x, p.y + dist } },
        { static_cast<float>(p.out_point), { p.x, p.y - dist } }
    };
    return l;
}

LayerSpec build_image_pan_down(const ImageParams& p) {
    auto l = make_image_layer_base(p);
    float dist = p.h * p.pan_distance;
    l.transform.position_property.value = { p.x, p.y - dist };
    l.transform.position_property.keyframes = {
        { static_cast<float>(p.in_point),  { p.x, p.y - dist } },
        { static_cast<float>(p.out_point), { p.x, p.y + dist } }
    };
    return l;
}

LayerSpec build_image_ken_burns(const ImageParams& p) {
    auto l = make_image_layer_base(p);
    // Combine zoom and slight pan
    float dist_x = p.w * 0.02f;
    float dist_y = p.h * 0.02f;
    
    l.transform.scale_property.keyframes = {
        { static_cast<float>(p.in_point),  { 105.0f, 105.0f } },
        { static_cast<float>(p.out_point), { 115.0f, 115.0f } }
    };
    l.transform.position_property.keyframes = {
        { static_cast<float>(p.in_point),  { p.x - dist_x, p.y - dist_y } },
        { static_cast<float>(p.out_point), { p.x + dist_x, p.y + dist_y } }
    };
    return l;
}

LayerSpec build_image(const ImageParams& p) {
    if (p.animation == "slow_zoom") return build_image_slow_zoom(p);
    if (p.animation == "pan_left")  return build_image_pan_left(p);
    if (p.animation == "pan_right") return build_image_pan_right(p);
    if (p.animation == "pan_up")    return build_image_pan_up(p);
    if (p.animation == "pan_down")  return build_image_pan_down(p);
    if (p.animation == "ken_burns") return build_image_ken_burns(p);
    return build_image_static(p);
}

} // namespace tachyon::presets
