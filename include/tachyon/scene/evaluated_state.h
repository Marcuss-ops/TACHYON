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
#include <string>
#include <vector>
#include <memory>

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
    Precomp
};

struct EvaluatedCompositionState;

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
    std::shared_ptr<EvaluatedCompositionState> nested_composition;
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
    EvaluatedCameraState camera;
};

} // namespace scene
} // namespace tachyon
