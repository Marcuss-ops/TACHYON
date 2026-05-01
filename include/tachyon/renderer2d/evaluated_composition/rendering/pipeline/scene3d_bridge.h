#pragma once

#include "tachyon/renderer3d/core/evaluated_scene_3d.h"
#include "tachyon/core/scene/state/evaluated_state.h"
#include "tachyon/runtime/execution/planning/render_plan.h"
#include "tachyon/renderer2d/resource/render_context.h"

#include <vector>
#include <optional>

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
};

/**
 * @brief Output from building the 3D scene bridge.
 */
struct Scene3DBridgeOutput {
    renderer3d::EvaluatedScene3D scene3d;
    bool has_instances{false};
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
