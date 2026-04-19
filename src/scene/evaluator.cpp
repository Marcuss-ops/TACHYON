#include "tachyon/scene/evaluator.h"

#include "tachyon/core/math/quaternion.h"
#include "tachyon/core/math/transform2.h"
#include "tachyon/timeline/time.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <optional>
#include <unordered_map>

namespace tachyon {
namespace scene {
namespace {

constexpr double kPi = 3.141592653589793238462643383279502884;

double degrees_to_radians(double degrees) {
    return degrees * kPi / 180.0;
}

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

math::Transform2 make_transform2(const math::Vector2& position, double rotation_degrees, const math::Vector2& scale) {
    math::Transform2 transform;
    transform.position = position;
    transform.rotation_rad = static_cast<float>(degrees_to_radians(rotation_degrees));
    transform.scale = scale;
    return transform;
}

EvaluatedLayerState make_layer_state(
    const LayerSpec& layer,
    std::size_t layer_index,
    std::int64_t frame_number,
    double composition_time_seconds) {
    EvaluatedLayerState evaluated;
    evaluated.layer_index = layer_index;
    evaluated.id = layer.id;
    evaluated.type = layer.type;
    evaluated.name = layer.name;
    evaluated.enabled = layer.enabled;
    evaluated.is_camera = layer.type == "camera";
    evaluated.frame_number = frame_number;
    evaluated.composition_time_seconds = composition_time_seconds;
    evaluated.local_time_seconds = timeline::local_time_from_composition(composition_time_seconds, layer.start_time);
    evaluated.remapped_time_seconds = sample_scalar(layer.time_remap_property, evaluated.local_time_seconds, evaluated.local_time_seconds);
    evaluated.active = layer.enabled && composition_time_seconds >= layer.in_point && composition_time_seconds <= layer.out_point;
    evaluated.opacity = sample_scalar(layer.opacity_property, layer.opacity, evaluated.remapped_time_seconds);
    evaluated.position = sample_vector2(layer.transform.position_property, fallback_position(layer), evaluated.remapped_time_seconds);
    evaluated.rotation_degrees = sample_scalar(layer.transform.rotation_property, fallback_rotation(layer), evaluated.remapped_time_seconds);
    evaluated.scale = sample_vector2(layer.transform.scale_property, fallback_scale(layer), evaluated.remapped_time_seconds);
    evaluated.parent = layer.parent;
    evaluated.local_transform = make_transform2(evaluated.position, evaluated.rotation_degrees, evaluated.scale);
    evaluated.local_matrix = evaluated.local_transform.to_matrix();
    evaluated.world_transform = evaluated.local_transform;
    evaluated.world_matrix = evaluated.local_matrix;
    evaluated.world_position = evaluated.position;
    evaluated.world_rotation_degrees = evaluated.rotation_degrees;
    evaluated.world_scale = evaluated.scale;
    evaluated.world_opacity = evaluated.opacity;
    evaluated.visible = evaluated.enabled && evaluated.active && evaluated.opacity > 0.0;
    return evaluated;
}

struct EvaluationContext {
    const CompositionSpec& composition;
    std::int64_t frame_number{0};
    double composition_time_seconds{0.0};
    std::unordered_map<std::string, std::size_t> layer_indices;
    std::vector<std::optional<EvaluatedLayerState>> cache;
    std::vector<bool> visiting;
};

const EvaluatedLayerState& resolve_layer_state(
    std::size_t layer_index,
    EvaluationContext& context) {
    if (context.cache[layer_index].has_value()) {
        return *context.cache[layer_index];
    }

    if (context.visiting[layer_index]) {
        context.cache[layer_index] = make_layer_state(
            context.composition.layers[layer_index],
            layer_index,
            context.frame_number,
            context.composition_time_seconds);
        return *context.cache[layer_index];
    }

    context.visiting[layer_index] = true;

    const auto& layer = context.composition.layers[layer_index];
    EvaluatedLayerState evaluated = make_layer_state(layer, layer_index, context.frame_number, context.composition_time_seconds);

    if (layer.parent.has_value() && !layer.parent->empty()) {
        const auto parent_it = context.layer_indices.find(*layer.parent);
        if (parent_it != context.layer_indices.end() && parent_it->second < layer_index) {
            const auto& parent = resolve_layer_state(parent_it->second, context);
            evaluated.parent_index = parent_it->second;
            evaluated.depth = parent.depth + 1;
            evaluated.world_matrix = parent.world_matrix * evaluated.local_matrix;
            evaluated.world_position = evaluated.world_matrix.transform_point({0.0f, 0.0f, 0.0f});
            evaluated.world_rotation_degrees = parent.world_rotation_degrees + evaluated.rotation_degrees;
            evaluated.world_scale = {
                parent.world_scale.x * evaluated.scale.x,
                parent.world_scale.y * evaluated.scale.y
            };
            evaluated.world_transform = make_transform2(
                evaluated.world_position,
                evaluated.world_rotation_degrees,
                evaluated.world_scale);
            evaluated.world_opacity = parent.world_opacity * evaluated.opacity;
            evaluated.visible = evaluated.enabled && evaluated.active && parent.visible && evaluated.world_opacity > 0.0;
        }
    }

    context.visiting[layer_index] = false;
    context.cache[layer_index] = std::move(evaluated);
    return *context.cache[layer_index];
}

} // namespace

EvaluatedLayerState evaluate_layer_state(const LayerSpec& layer, std::int64_t frame_number, double composition_time_seconds) {
    return make_layer_state(layer, 0, frame_number, composition_time_seconds);
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
        evaluated.position = {it->world_position.x, it->world_position.y, 0.0f};
        evaluated.rotation_degrees = it->world_rotation_degrees;
        evaluated.scale = {it->world_scale.x, it->world_scale.y, 1.0f};
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

    EvaluationContext context{
        composition,
        frame_number,
        frame.composition_time_seconds,
        {},
        std::vector<std::optional<EvaluatedLayerState>>(composition.layers.size()),
        std::vector<bool>(composition.layers.size(), false)
    };

    for (std::size_t index = 0; index < composition.layers.size(); ++index) {
        context.layer_indices.emplace(composition.layers[index].id, index);
    }

    for (std::size_t index = 0; index < composition.layers.size(); ++index) {
        evaluated.layers.push_back(resolve_layer_state(index, context));
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
