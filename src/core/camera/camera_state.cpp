#include "tachyon/core/camera/camera_state.h"

namespace tachyon {
namespace camera {

math::Vector2 CameraState::project_point(const math::Vector3& world_p, float viewport_w, float viewport_h) const {
    math::Matrix4x4 view = get_view_matrix();
    math::Matrix4x4 proj = get_projection_matrix();
    
    // 1. World -> Camera Space
    math::Vector3 camera_p = view.transform_point(world_p);
    
    // Behind camera check
    if (camera_p.z > -near_z) {
        return { -1.0f, -1.0f };
    }
    
    // 2. Camera -> Clip Space (manual perspective divide)
    // We don't use the full matrix transform here for clarity and to handle the W divide
    float tan_half_fov = std::tan(fov_y_rad * 0.5f);
    
    // Projection properties from perspective matrix logic:
    // x' = x / (aspect * tan * -z)
    // y' = y / (tan * -z)
    float w = -camera_p.z;
    float ndc_x = camera_p.x / (aspect * tan_half_fov * w);
    float ndc_y = camera_p.y / (tan_half_fov * w);
    
    // 3. NDC [-1, 1] -> Viewport [0, W], [0, H]
    // Note: Y is flipped because Raster Y is down
    float screen_x = (ndc_x + 1.0f) * 0.5f * viewport_w;
    float screen_y = (1.0f - ndc_y) * 0.5f * viewport_h;
    
    return { screen_x, screen_y };
}

} // namespace camera
} // namespace tachyon
