#pragma once

// Forward declarations for types needed by evaluator.cpp
namespace tachyon { namespace math {
    struct Vector2;
    struct Vector3;
    struct Quaternion;
    struct Matrix4x4;
    struct Transform2;
} }

namespace tachyon {
    struct LayerSpec;
    struct CompositionSpec;
    struct SceneSpec;
    struct Camera2DSpec;
}

namespace tachyon { namespace scene {

using PropertySampler = std::function<double(int, double)>;

using LayerSpec = ::tachyon::LayerSpec;
using CompositionSpec = ::tachyon::CompositionSpec;
using SceneSpec = ::tachyon::SceneSpec;
using Camera2DSpec = ::tachyon::Camera2DSpec;

} // namespace scene
} // namespace tachyon
