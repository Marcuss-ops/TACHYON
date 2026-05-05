#pragma once
#include <vector>
#include <optional>
#include <memory>
#include <string>
#include <unordered_map>
#include <cstdint>

#include "tachyon/core/math/matrix4x4.h"
#include "tachyon/core/types/color_spec.h"

#ifdef TACHYON_ENABLE_3D
#include "tachyon/renderer3d/core/evaluated_scene_3d.h"
#else
namespace tachyon::renderer3d {
    struct EvaluatedCamera3D {};
    struct EvaluatedLight {};
    struct EvaluatedMeshInstance {
        std::uint32_t object_id;
        std::uint32_t material_id;
        ::tachyon::math::Matrix4x4 world_transform;
        std::optional<::tachyon::math::Matrix4x4> previous_world_transform;
        std::string mesh_asset_id;
        std::shared_ptr<const void> mesh_asset; // Use void* for stub
        
        struct MaterialStub {
            ::tachyon::ColorSpec base_color;
            float opacity;
            float metallic;
            float roughness;
            float emission_strength;
            ::tachyon::ColorSpec emission_color;
            float transmission;
            float ior;
        } material;

        std::vector<::tachyon::math::Matrix4x4> joint_matrices;
        std::vector<float> morph_weights;
    };
    struct EvaluatedScene3D {
        EvaluatedCamera3D camera;
        std::vector<EvaluatedLight> lights;
        std::vector<EvaluatedMeshInstance> instances;
        std::string environment_map_id;
    };
}
#endif
#include "tachyon/core/scene/state/evaluated_state.h"
#include "tachyon/runtime/execution/planning/render_plan.h"
#include "tachyon/renderer2d/resource/render_context.h"
#include "tachyon/renderer2d/core/framebuffer.h"

namespace tachyon {

using renderer2d::RenderContext2D;

/**
 * @brief Input data for building the 3D scene bridge.
 */
struct Scene3DBridgeInput {
    const scene::EvaluatedCompositionState* state{nullptr};
    const RenderPlan* plan{nullptr};
    const FrameRenderTask* task{nullptr};
    const renderer2d::RenderContext2D* context{nullptr};
    const std::vector<std::size_t>* block_indices{nullptr};
    const std::vector<bool>* visible_3d_layers{nullptr};
    const std::unordered_map<std::string, std::shared_ptr<renderer2d::SurfaceRGBA>>* rendered_surfaces{nullptr};
};

/**
 * @brief Output from building the 3D scene bridge.
 */
struct Scene3DBridgeOutput {
    renderer3d::EvaluatedScene3D scene3d;
    bool has_instances{false};
};

/**
 * @brief Common 2D surface representation used by the 3D bridge.
 */
struct LayerSurface {
    std::shared_ptr<const renderer2d::SurfaceRGBA> surface;
    std::string cache_key;
    std::string source_kind;

    [[nodiscard]] bool valid() const noexcept {
        return surface != nullptr;
    }
};

/**
 * @brief Builds a renderer3d::EvaluatedScene3D from 2D composition state.
 *
 * This is the single bridge function that translates:
 * - scene::EvaluatedCompositionState
 * - RenderPlan / FrameRenderTask
 * - RenderContext2D (fonts, media, etc.)
 *
 * Into the 3D contract: renderer3d::EvaluatedScene3D
 *
 * This replaces the inline bridge code that was in composition_renderer.cpp.
 */
Scene3DBridgeOutput build_evaluated_scene_3d(const Scene3DBridgeInput& input);

/**
 * @brief Build the shared 2D surface used by the 3D bridge for a layer.
 */
LayerSurface build_layer_surface(
    const scene::EvaluatedLayerState& layer,
    const Scene3DBridgeInput& input);

/**
 * @brief Sub-step: Build camera for 3D scene.
 */
renderer3d::EvaluatedCamera3D build_camera_3d(
    const scene::EvaluatedCameraState& camera,
    const scene::EvaluatedCompositionState& state);

/**
 * @brief Sub-step: Build lights for 3D scene.
 */
std::vector<renderer3d::EvaluatedLight> build_lights_3d(
    const std::vector<scene::EvaluatedLightState>& lights);

/**
 * @brief Sub-step: Build mesh instances for 3D scene.
 */
std::vector<renderer3d::EvaluatedMeshInstance> build_instances_3d(
    const Scene3DBridgeInput& input);

} // namespace tachyon
