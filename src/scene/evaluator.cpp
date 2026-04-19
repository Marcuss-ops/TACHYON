#include "tachyon/scene/evaluator.h"

#include "tachyon/core/math/quaternion.h"
#include "tachyon/timeline/time.h"

#include <algorithm>

namespace tachyon {
namespace scene {
namespace {

math::Vector2 fallback_position(const LayerSpec& layer) {
    return {
        static_cast<float>(layer.transform.position_x.value_or(0.0)),
        static_cast<float>(layer.transform.position_y.value_or(0.0))
    };
}

math::Vector2 fallback_scale(const LayerSpec& layer) {
    return {
        static_cast<float>(layer.transform.scale_x.value_or(1.0)),
        static_cast<float>(layer.transform.scale_y.value_or(1.0))
    };
}

double fallback_rotation(const LayerSpec& layer) {
    return layer.transform.rotation.value_or(0.0);
}

double sample_scalar(const AnimatedScalarSpec& property, double fallback, double local_time_seconds) {
    if (property.keyframes.empty()) {
        return property.value.value_or(fallback);
    }

    std::vector<ScalarKeyframeSpec> keyframes = property.keyframes;
    std::stable_sort(keyframes.begin(), keyframes.end(), [](const auto& a, const auto& b) {
        return a.time < b.time;
    });

    if (local_time_seconds <= keyframes.front().time) {
        return keyframes.front().value;
    }
    if (local_time_seconds >= keyframes.back().time) {
        return keyframes.back().value;
    }

    for (std::size_t index = 1; index < keyframes.size(); ++index) {
        const auto& previous = keyframes[index - 1];
        const auto& next = keyframes[index];
        if (local_time_seconds > next.time) {
            continue;
        }
        const double duration = next.time - previous.time;
        if (duration <= 0.0) {
            return next.value;
        }
        const double alpha = (local_time_seconds - previous.time) / duration;
        return previous.value + (next.value - previous.value) * alpha;
    }

    return keyframes.back().value;
}

math::Vector2 sample_vector2(const AnimatedVector2Spec& property, const math::Vector2& fallback, double local_time_seconds) {
    if (property.keyframes.empty()) {
        return property.value.value_or(fallback);
    }

    std::vector<Vector2KeyframeSpec> keyframes = property.keyframes;
    std::stable_sort(keyframes.begin(), keyframes.end(), [](const auto& a, const auto& b) {
        return a.time < b.time;
    });

    if (local_time_seconds <= keyframes.front().time) {
        return keyframes.front().value;
    }
    if (local_time_seconds >= keyframes.back().time) {
        return keyframes.back().value;
    }

    for (std::size_t index = 1; index < keyframes.size(); ++index) {
        const auto& previous = keyframes[index - 1];
        const auto& next = keyframes[index];
        if (local_time_seconds > next.time) {
            continue;
        }
        const double duration = next.time - previous.time;
        if (duration <= 0.0) {
            return next.value;
        }
        const float alpha = static_cast<float>((local_time_seconds - previous.time) / duration);
        return previous.value * (1.0f - alpha) + next.value * alpha;
    }

    return keyframes.back().value;
}

} // namespace

EvaluatedLayerState evaluate_layer_state(const LayerSpec& layer, std::int64_t frame_number, double composition_time_seconds) {
    EvaluatedLayerState evaluated;
    evaluated.id = layer.id;
    evaluated.type = layer.type;
    evaluated.name = layer.name;
    evaluated.enabled = layer.enabled;
    evaluated.is_camera = layer.type == "camera";
    evaluated.frame_number = frame_number;
    evaluated.composition_time_seconds = composition_time_seconds;
    evaluated.local_time_seconds = timeline::local_time_from_composition(composition_time_seconds, layer.start_time);
    evaluated.active = layer.enabled && composition_time_seconds >= layer.in_point && composition_time_seconds <= layer.out_point;
    evaluated.opacity = sample_scalar(layer.opacity_property, layer.opacity, evaluated.local_time_seconds);
    evaluated.position = sample_vector2(layer.transform.position_property, fallback_position(layer), evaluated.local_time_seconds);
    evaluated.rotation_degrees = sample_scalar(layer.transform.rotation_property, fallback_rotation(layer), evaluated.local_time_seconds);
    evaluated.scale = sample_vector2(layer.transform.scale_property, fallback_scale(layer), evaluated.local_time_seconds);
    evaluated.parent = layer.parent;
    return evaluated;
}

EvaluatedCameraState evaluate_camera_state(
    const CompositionSpec& composition,
    const std::vector<EvaluatedLayerState>& layers,
    std::int64_t frame_number,
    double composition_time_seconds
) {
    EvaluatedCameraState evaluated;
    evaluated.frame_number = frame_number;
    evaluated.composition_time_seconds = composition_time_seconds;
    evaluated.camera.aspect = composition.height > 0
        ? static_cast<float>(static_cast<double>(composition.width) / static_cast<double>(composition.height))
        : 1.0f;

    for (auto it = layers.rbegin(); it != layers.rend(); ++it) {
        if (!it->is_camera || !it->active) {
            continue;
        }

        evaluated.available = true;
        evaluated.layer_id = it->id;
        evaluated.local_time_seconds = it->local_time_seconds;
        evaluated.position = {it->position.x, it->position.y, 0.0f};
        evaluated.rotation_degrees = it->rotation_degrees;
        evaluated.scale = {it->scale.x, it->scale.y, 1.0f};
        evaluated.camera.transform.compose_trs(
            evaluated.position,
            math::Quaternion::from_euler({0.0f, 0.0f, static_cast<float>(evaluated.rotation_degrees)}),
            evaluated.scale
        );
        break;
    }

    return evaluated;
}

EvaluatedCompositionState evaluate_composition_state(const CompositionSpec& composition, std::int64_t frame_number) {
    const auto frame = timeline::sample_frame(composition.frame_rate, frame_number);

    EvaluatedCompositionState evaluated;
    evaluated.composition_id = composition.id;
    evaluated.composition_name = composition.name;
    evaluated.width = composition.width;
    evaluated.height = composition.height;
    evaluated.frame_rate = composition.frame_rate;
    evaluated.frame_number = frame.frame_number;
    evaluated.composition_time_seconds = frame.composition_time_seconds;
    evaluated.layers.reserve(composition.layers.size());

    for (const auto& layer : composition.layers) {
        evaluated.layers.push_back(evaluate_layer_state(layer, frame_number, frame.composition_time_seconds));
    }

    evaluated.camera = evaluate_camera_state(composition, evaluated.layers, frame_number, frame.composition_time_seconds);
    return evaluated;
}

std::optional<EvaluatedCompositionState> evaluate_scene_composition_state(
    const SceneSpec& scene,
    const std::string& composition_id,
    std::int64_t frame_number
) {
    for (const auto& composition : scene.compositions) {
        if (composition.id == composition_id) {
            return evaluate_composition_state(composition, frame_number);
        }
    }
    return std::nullopt;
}

} // namespace scene
} // namespace tachyon
