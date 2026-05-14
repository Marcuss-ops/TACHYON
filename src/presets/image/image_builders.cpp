#include "tachyon/presets/image/image_builders.h"
#include "tachyon/presets/transition/transition_builders.h"
#include "tachyon/core/animation/easing.h"

namespace tachyon::presets {

namespace {

LayerSpec make_image_layer_base(const ImageParams& p) {
    LayerSpec l;
    l.identity.id          = "image_layer";
    l.identity.name        = "Image";
    l.identity.type        = LayerType::Image;
    l.identity.enabled     = true;
    l.identity.visible     = true;
    l.playback.timing.start     = p.in_point;
    l.playback.timing.source_in = p.in_point;
    l.playback.timing.duration  = std::max(0.01, p.out_point - p.in_point);
    l.transform.width       = static_cast<int>(p.w);
    l.transform.height      = static_cast<int>(p.h);
    l.transform.opacity     = static_cast<double>(p.opacity);
    l.source.asset_id    = p.path; // We use the path as the asset ID reference
    
    l.transform.transform.position_x = static_cast<double>(p.x);
    l.transform.transform.position_y = static_cast<double>(p.y);
    l.transform.transform.anchor_point.value = { p.w * 0.5f, p.h * 0.5f };

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
    l.transform.transform.scale_property.value = { p.zoom_from * 100.0f, p.zoom_from * 100.0f };
    l.transform.transform.scale_property.keyframes.push_back(Vector2KeyframeSpec{
        static_cast<double>(p.in_point), { p.zoom_from * 100.0f, p.zoom_from * 100.0f }
    });
    l.transform.transform.scale_property.keyframes.push_back(Vector2KeyframeSpec{
        static_cast<double>(p.out_point), { p.zoom_to * 100.0f, p.zoom_to * 100.0f }
    });
    return l;
}

LayerSpec build_image_pan_left(const ImageParams& p) {
    auto l = make_image_layer_base(p);
    float dist = p.w * p.pan_distance;
    l.transform.transform.position_property.value = { p.x + dist, p.y };
    l.transform.transform.position_property.keyframes.push_back(Vector2KeyframeSpec{
        static_cast<double>(p.in_point), { p.x + dist, p.y }
    });
    l.transform.transform.position_property.keyframes.push_back(Vector2KeyframeSpec{
        static_cast<double>(p.out_point), { p.x - dist, p.y }
    });
    return l;
}

LayerSpec build_image_pan_right(const ImageParams& p) {
    auto l = make_image_layer_base(p);
    float dist = p.w * p.pan_distance;
    l.transform.transform.position_property.value = { p.x - dist, p.y };
    l.transform.transform.position_property.keyframes.push_back(Vector2KeyframeSpec{
        static_cast<double>(p.in_point), { p.x - dist, p.y }
    });
    l.transform.transform.position_property.keyframes.push_back(Vector2KeyframeSpec{
        static_cast<double>(p.out_point), { p.x + dist, p.y }
    });
    return l;
}

LayerSpec build_image_pan_up(const ImageParams& p) {
    auto l = make_image_layer_base(p);
    float dist = p.h * p.pan_distance;
    l.transform.transform.position_property.value = { p.x, p.y + dist };
    l.transform.transform.position_property.keyframes.push_back(Vector2KeyframeSpec{
        static_cast<double>(p.in_point), { p.x, p.y + dist }
    });
    l.transform.transform.position_property.keyframes.push_back(Vector2KeyframeSpec{
        static_cast<double>(p.out_point), { p.x, p.y - dist }
    });
    return l;
}

LayerSpec build_image_pan_down(const ImageParams& p) {
    auto l = make_image_layer_base(p);
    float dist = p.h * p.pan_distance;
    l.transform.transform.position_property.value = { p.x, p.y - dist };
    l.transform.transform.position_property.keyframes.push_back(Vector2KeyframeSpec{
        static_cast<double>(p.in_point), { p.x, p.y - dist }
    });
    l.transform.transform.position_property.keyframes.push_back(Vector2KeyframeSpec{
        static_cast<double>(p.out_point), { p.x, p.y + dist }
    });
    return l;
}

LayerSpec build_image_ken_burns(const ImageParams& p) {
    auto l = make_image_layer_base(p);
    // Combine zoom and slight pan
    float dist_x = p.w * 0.02f;
    float dist_y = p.h * 0.02f;
    
    l.transform.transform.scale_property.keyframes.push_back(Vector2KeyframeSpec{
        static_cast<double>(p.in_point), { 105.0f, 105.0f }
    });
    l.transform.transform.scale_property.keyframes.push_back(Vector2KeyframeSpec{
        static_cast<double>(p.out_point), { 115.0f, 115.0f }
    });
    l.transform.transform.position_property.keyframes.push_back(Vector2KeyframeSpec{
        static_cast<double>(p.in_point), { p.x - dist_x, p.y - dist_y }
    });
    l.transform.transform.position_property.keyframes.push_back(Vector2KeyframeSpec{
        static_cast<double>(p.out_point), { p.x + dist_x, p.y + dist_y }
    });
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
