#include "tachyon/timeline/evaluator.h"
#include "tachyon/core/camera/camera_state.h"

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
        if (time_seconds <= spec.keyframes.front().time) return spec.keyframes.front().value;
        for (std::size_t i = 1; i < spec.keyframes.size(); ++i) {
            const auto& a = spec.keyframes[i - 1];
            const auto& b = spec.keyframes[i];
            if (time_seconds <= b.time) {
                const double span = b.time - a.time;
                if (span <= 0.0) return b.value;
                const double t = (time_seconds - a.time) / span;
                return a.value + (b.value - a.value) * t;
            }
        }
        return spec.keyframes.back().value;
    }
    return spec.value.value_or(fallback);
}

math::Vector2 evaluate_vector2(const AnimatedVector2Spec& spec, double time_seconds, const math::Vector2& fallback) {
    if (!spec.keyframes.empty()) {
        if (time_seconds <= spec.keyframes.front().time) return spec.keyframes.front().value;
        for (std::size_t i = 1; i < spec.keyframes.size(); ++i) {
            const auto& a = spec.keyframes[i - 1];
            const auto& b = spec.keyframes[i];
            if (time_seconds <= b.time) {
                const double span = b.time - a.time;
                if (span <= 0.0) return b.value;
                const float t = static_cast<float>((time_seconds - a.time) / span);
                return a.value + (b.value - a.value) * t;
            }
        }
        return spec.keyframes.back().value;
    }
    return spec.value.value_or(fallback);
}

EvaluatedLayerState evaluate_layer(const LayerSpec& layer, double composition_time_seconds, double composition_duration, int z_order) {
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

    if (out.type == LayerType::Text) out.text = EvaluatedTextPayload{layer.name};
    if (out.type == LayerType::Image) out.image = EvaluatedImagePayload{layer.id};
    if (out.type == LayerType::Solid || out.type == LayerType::Shape) out.solid = EvaluatedSolidPayload{"default"};
    return out;
}

EvaluatedCameraState evaluate_camera(const CompositionSpec& composition, const std::vector<EvaluatedLayerState>& layers) {
    EvaluatedCameraState camera_state;
    camera::CameraState camera_runtime;
    camera_runtime.aspect = (composition.height != 0) ? static_cast<float>(composition.width) / static_cast<float>(composition.height) : 1.0f;

    for (const auto& layer : layers) {
        if (layer.type == LayerType::Camera && layer.visible) {
            camera_state.enabled = true;
            camera_state.position = layer.transform3.position;
            camera_state.rotation = layer.transform3.rotation;
            camera_runtime.transform.position = camera_state.position;
            camera_runtime.transform.rotation = camera_state.rotation;
            break;
        }
    }

    camera_state.fov_y_rad = camera_runtime.fov_y_rad;
    camera_state.near_z = camera_runtime.near_z;
    camera_state.far_z = camera_runtime.far_z;
    camera_state.view = camera_runtime.get_view_matrix();
    camera_state.projection = camera_runtime.get_projection_matrix();
    return camera_state;
}

} // namespace

std::optional<EvaluatedCompositionState> evaluate_composition_frame(const EvaluationRequest& request) {
    if (!request.scene) {
        return std::nullopt;
    }

    const CompositionSpec* composition = nullptr;
    for (const auto& candidate : request.scene->compositions) {
        if (candidate.id == request.composition_id) {
            composition = &candidate;
            break;
        }
    }
    if (!composition) {
        return std::nullopt;
    }

    EvaluatedCompositionState state;
    state.composition_id = composition->id;
    state.width = static_cast<int>(composition->width);
    state.height = static_cast<int>(composition->height);
    state.time_seconds = frame_to_seconds(request.frame_number, composition->frame_rate);

    state.layers.reserve(composition->layers.size());
    for (std::size_t i = 0; i < composition->layers.size(); ++i) {
        state.layers.push_back(evaluate_layer(composition->layers[i], state.time_seconds, composition->duration, static_cast<int>(i)));
    }

    state.camera = evaluate_camera(*composition, state.layers);
    return state;
}

} // namespace tachyon::timeline
