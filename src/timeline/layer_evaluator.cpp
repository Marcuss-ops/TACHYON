#include "tachyon/timeline/evaluator.h"

#include <algorithm>
#include <cmath>

namespace tachyon::timeline {
namespace {

constexpr float kDegToRad = 3.14159265f / 180.0f;

LayerType parse_layer_type(const std::string& type) {
    if (type == "solid") return LayerType::Solid;
    if (type == "shape") return LayerType::Shape;
    if (type == "mask") return LayerType::Mask;
    if (type == "image") return LayerType::Image;
    if (type == "text") return LayerType::Text;
    if (type == "camera") return LayerType::Camera;
    return LayerType::Unknown;
}

double evaluate_scalar(const AnimatedScalarSpec& spec, double time_seconds, double fallback) {
    if (!spec.keyframes.empty()) {
        if (time_seconds <= spec.keyframes.front().time) {
            return spec.keyframes.front().value;
        }
        for (std::size_t i = 1; i < spec.keyframes.size(); ++i) {
            const auto& prev = spec.keyframes[i - 1];
            const auto& next = spec.keyframes[i];
            if (time_seconds <= next.time) {
                const double span = next.time - prev.time;
                if (span <= 0.0) {
                    return next.value;
                }
                const double t = (time_seconds - prev.time) / span;
                return prev.value + (next.value - prev.value) * t;
            }
        }
        return spec.keyframes.back().value;
    }
    return spec.value.value_or(fallback);
}

math::Vector2 evaluate_vector2(const AnimatedVector2Spec& spec, double time_seconds, const math::Vector2& fallback) {
    if (!spec.keyframes.empty()) {
        if (time_seconds <= spec.keyframes.front().time) {
            return spec.keyframes.front().value;
        }
        for (std::size_t i = 1; i < spec.keyframes.size(); ++i) {
            const auto& prev = spec.keyframes[i - 1];
            const auto& next = spec.keyframes[i];
            if (time_seconds <= next.time) {
                const double span = next.time - prev.time;
                if (span <= 0.0) {
                    return next.value;
                }
                const float t = static_cast<float>((time_seconds - prev.time) / span);
                return prev.value + (next.value - prev.value) * t;
            }
        }
        return spec.keyframes.back().value;
    }
    return spec.value.value_or(fallback);
}

EvaluatedLayerState evaluate_layer_state(const LayerSpec& layer, double composition_time_seconds, double composition_duration, int z_order) {
    EvaluatedLayerState out;
    out.id = layer.id;
    out.type = parse_layer_type(layer.type);
    out.visible = is_layer_visible_at_time(layer, composition_time_seconds, composition_duration);
    out.local_time_seconds = composition_time_seconds - layer.start_time;
    out.z_order = z_order;

    out.opacity = static_cast<float>(evaluate_scalar(layer.opacity_property, out.local_time_seconds, layer.opacity));

    const math::Vector2 position = evaluate_vector2(layer.transform.position_property, out.local_time_seconds,
        math::Vector2{static_cast<float>(layer.transform.position_x.value_or(0.0)), static_cast<float>(layer.transform.position_y.value_or(0.0))});
    const float rotation_deg = static_cast<float>(evaluate_scalar(layer.transform.rotation_property, out.local_time_seconds, layer.transform.rotation.value_or(0.0)));
    const math::Vector2 scale = evaluate_vector2(layer.transform.scale_property, out.local_time_seconds,
        math::Vector2{static_cast<float>(layer.transform.scale_x.value_or(1.0)), static_cast<float>(layer.transform.scale_y.value_or(1.0))});

    out.transform2.position = position;
    out.transform2.rotation_rad = rotation_deg * kDegToRad;
    out.transform2.scale = scale;

    out.transform3.position = {position.x, position.y, 0.0f};
    out.transform3.rotation = math::Quaternion::from_euler({0.0f, 0.0f, rotation_deg});
    out.transform3.scale = {scale.x, scale.y, 1.0f};

    if (out.type == LayerType::Text) {
        out.text = EvaluatedTextPayload{layer.name};
    } else if (out.type == LayerType::Image) {
        out.image = EvaluatedImagePayload{layer.id};
    } else if (out.type == LayerType::Solid || out.type == LayerType::Shape) {
        out.solid = EvaluatedSolidPayload{"default"};
    }

    return out;
}

} // namespace
} // namespace tachyon::timeline
