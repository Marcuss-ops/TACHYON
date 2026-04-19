#pragma once

#include "tachyon/core/math/matrix4x4.h"
#include "tachyon/core/math/quaternion.h"
#include "tachyon/core/math/transform2.h"
#include "tachyon/core/math/transform3.h"
#include "tachyon/core/math/vector2.h"
#include "tachyon/core/math/vector3.h"

#include <optional>
#include <string>
#include <vector>

namespace tachyon::timeline {

enum class LayerType {
    Unknown,
    Solid,
    Image,
    Text,
    Camera,
};

struct EvaluatedTextPayload {
    std::string text;
};

struct EvaluatedImagePayload {
    std::string asset_id;
};

struct EvaluatedSolidPayload {
    std::string color;
};

struct EvaluatedCameraState {
    bool enabled{false};
    math::Vector3 position{math::Vector3::zero()};
    math::Quaternion rotation{math::Quaternion::identity()};
    float fov_y_rad{0.785398f};
    float near_z{0.1f};
    float far_z{1000.0f};
    math::Matrix4x4 view{math::Matrix4x4::identity()};
    math::Matrix4x4 projection{math::Matrix4x4::identity()};
};

struct EvaluatedLayerState {
    std::string id;
    LayerType type{LayerType::Unknown};
    bool visible{false};
    float opacity{1.0f};
    double local_time_seconds{0.0};
    math::Transform2 transform2;
    math::Transform3 transform3;
    int z_order{0};
    std::optional<EvaluatedTextPayload> text;
    std::optional<EvaluatedImagePayload> image;
    std::optional<EvaluatedSolidPayload> solid;
};

struct EvaluatedCompositionState {
    std::string composition_id;
    int width{0};
    int height{0};
    double time_seconds{0.0};
    EvaluatedCameraState camera;
    std::vector<EvaluatedLayerState> layers;
};

} // namespace tachyon::timeline
