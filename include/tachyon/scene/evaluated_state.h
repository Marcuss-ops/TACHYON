#pragma once

#include "tachyon/core/camera/camera_state.h"
#include "tachyon/core/math/matrix4x4.h"
#include "tachyon/core/math/transform2.h"
#include "tachyon/core/math/vector2.h"
#include "tachyon/core/math/vector3.h"
#include "tachyon/spec/scene_spec.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <memory>
#include <string>
#include <vector>

namespace tachyon {
namespace scene {

enum class LayerType {
    Unknown,
    Solid,
    Shape,
    Mask,
    Image,
    Text,
    Camera,
    Precomp,
    Light
};

struct EvaluatedCompositionState;

struct EvaluatedLightState {
    std::string layer_id;
    std::string type; // "ambient", "parallel", "point", "spot"
    math::Vector3 position{math::Vector3::zero()};
    math::Vector3 direction{0.0f, 0.0f, -1.0f};
    ColorSpec color{255, 255, 255, 255};
    float intensity{1.0f};
    float attenuation_near{0.0f};
    float attenuation_far{1000.0f};
};

struct EvaluatedShapePathPoint {
    math::Vector2 position{math::Vector2::zero()};
    math::Vector2 tangent_in{math::Vector2::zero()};
    math::Vector2 tangent_out{math::Vector2::zero()};
};

struct EvaluatedShapePath {
    std::vector<EvaluatedShapePathPoint> points;
    bool closed{true};
};

struct EvaluatedLayerState {
    EvaluatedLayerState() = default;
    EvaluatedLayerState(const EvaluatedLayerState& other);
    EvaluatedLayerState& operator=(const EvaluatedLayerState& other);
    EvaluatedLayerState(EvaluatedLayerState&&) noexcept = default;
    EvaluatedLayerState& operator=(EvaluatedLayerState&&) noexcept = default;

    std::size_t layer_index{0};
    std::string id;
    std::string name;
    LayerType type{LayerType::Unknown};
    std::string blend_mode{"normal"};
    bool enabled{false};
    bool active{false};
    bool visible{false};
    bool is_3d{false};
    bool is_adjustment_layer{false};
    
    double local_time_seconds{0.0};
    double child_time_seconds{0.0};
    double opacity{1.0};
    
    math::Transform2 local_transform{math::Transform2::identity()};
    math::Matrix4x4 world_matrix{math::Matrix4x4::identity()};
    
    // 3D Specific evaluated state
    math::Vector3 world_position3{math::Vector3::zero()};
    
    std::int64_t width{0};
    std::int64_t height{0};
    
    std::string text_content;
    ColorSpec fill_color{255, 255, 255, 255};
    
    std::optional<EvaluatedShapePath> shape_path;
    std::vector<EffectSpec> effects;
    
    TrackMatteType track_matte_type{TrackMatteType::None};
    std::optional<std::size_t> track_matte_layer_index;

    std::optional<std::string> precomp_id;
    std::unique_ptr<EvaluatedCompositionState> nested_composition;
};

struct EvaluatedCameraState {
    bool available{false};
    std::string layer_id;
    math::Vector3 position{math::Vector3::zero()};
    camera::CameraState camera;
};

struct EvaluatedCompositionState {
    std::string composition_id;
    std::string composition_name;
    std::int64_t width{0};
    std::int64_t height{0};
    FrameRate frame_rate;
    std::int64_t frame_number{0};
    double composition_time_seconds{0.0};
    std::vector<EvaluatedLayerState> layers;
    std::vector<EvaluatedLightState> lights;
    EvaluatedCameraState camera;
};

inline EvaluatedLayerState::EvaluatedLayerState(const EvaluatedLayerState& other)
    : layer_index(other.layer_index),
      id(other.id),
      name(other.name),
      type(other.type),
      blend_mode(other.blend_mode),
      enabled(other.enabled),
      active(other.active),
      visible(other.visible),
      is_3d(other.is_3d),
      is_adjustment_layer(other.is_adjustment_layer),
      local_time_seconds(other.local_time_seconds),
      child_time_seconds(other.child_time_seconds),
      opacity(other.opacity),
      local_transform(other.local_transform),
      world_matrix(other.world_matrix),
      world_position3(other.world_position3),
      width(other.width),
      height(other.height),
      text_content(other.text_content),
      fill_color(other.fill_color),
      shape_path(other.shape_path),
      effects(other.effects),
      track_matte_type(other.track_matte_type),
      track_matte_layer_index(other.track_matte_layer_index),
      precomp_id(other.precomp_id) {
    if (other.nested_composition) {
        nested_composition = std::make_unique<EvaluatedCompositionState>(*other.nested_composition);
    }
}

inline EvaluatedLayerState& EvaluatedLayerState::operator=(const EvaluatedLayerState& other) {
    if (this == &other) {
        return *this;
    }

    layer_index = other.layer_index;
    id = other.id;
    name = other.name;
    type = other.type;
    blend_mode = other.blend_mode;
    enabled = other.enabled;
    active = other.active;
    visible = other.visible;
    is_3d = other.is_3d;
    is_adjustment_layer = other.is_adjustment_layer;
    local_time_seconds = other.local_time_seconds;
    child_time_seconds = other.child_time_seconds;
    opacity = other.opacity;
    local_transform = other.local_transform;
    world_matrix = other.world_matrix;
    world_position3 = other.world_position3;
    width = other.width;
    height = other.height;
    text_content = other.text_content;
    fill_color = other.fill_color;
    shape_path = other.shape_path;
    effects = other.effects;
    track_matte_type = other.track_matte_type;
    track_matte_layer_index = other.track_matte_layer_index;
    precomp_id = other.precomp_id;
    if (other.nested_composition) {
        nested_composition = std::make_unique<EvaluatedCompositionState>(*other.nested_composition);
    } else {
        nested_composition.reset();
    }

    return *this;
}

} // namespace scene
} // namespace tachyon
