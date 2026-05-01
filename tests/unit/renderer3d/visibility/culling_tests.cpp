#include <gtest/gtest.h>

#include "tachyon/core/scene/state/evaluated_state.h"
#include "tachyon/core/math/algebra/quaternion.h"
#include "tachyon/renderer3d/visibility/culling.h"

using namespace tachyon;

TEST(SceneCulling, CameraOffsetKeepsCentered3DLayerVisible) {
    scene::EvaluatedCompositionState state;
    state.width = 1920;
    state.height = 1080;

    scene::EvaluatedLayerState layer;
    layer.id = "card";
    layer.type = scene::LayerType::Solid;
    layer.visible = true;
    layer.is_3d = true;
    layer.enabled = true;
    layer.active = true;
    layer.world_position3 = {960.0f, 540.0f, 0.0f};
    layer.scale_3d = {400.0f, 400.0f, 1.0f};
    state.layers.push_back(layer);

    state.camera.available = true;
    state.camera.position = {960.0f, 540.0f, -1000.0f};
    state.camera.point_of_interest = {960.0f, 540.0f, 0.0f};
    state.camera.camera.use_target = true;
    state.camera.camera.target_position = state.camera.point_of_interest;
    state.camera.camera.transform.position = state.camera.position;
    state.camera.camera.transform.rotation = math::Quaternion::identity();
    state.camera.camera.fov_y_rad = 0.75f;
    state.camera.camera.aspect = 16.0f / 9.0f;
    state.camera.camera.near_z = 0.1f;
    state.camera.camera.far_z = 5000.0f;

    renderer3d::SceneCulling culling;
    culling.set_camera(state.camera);

    const auto result = culling.cull_layers(state);
    ASSERT_EQ(result.visibleIndices.size(), 1U);
    EXPECT_EQ(result.visibleIndices.front(), 0U);
}
