#include "tachyon/tracker/core/track_binding.h"
#include "tachyon/tracker/core/track.h"
#include <cmath>

namespace tachyon::tracker {

TrackBinding::TrackBinding(const Track& track, TrackBindingTarget target)
    : m_track(&track), m_target(target) {}

void TrackBinding::apply(float time, scene::EvaluatedLayerState& layer) const {
    if (!m_track) return;

    auto pos = m_track->position_at(time);
    if (!pos.has_value()) return; // Silently fail if no data
    
    switch (m_target) {
        case TrackBindingTarget::Position:
            // Apply tracked position to layer's local transform
            layer.local_transform.position.x = pos->x;
            layer.local_transform.position.y = pos->y;
            break;
            
        case TrackBindingTarget::Rotation:
            // Use affine solve to extract rotation if available
            if (auto affine = m_track->affine_at(time)) {
                const auto& a = *affine; // [a, b, tx, c, d, ty]
                // Matrix layout is row-major: [a, b, tx, c, d, ty].
                // Rotation is derived from the first column.
                float angle = std::atan2(a[3], a[0]);
                layer.local_transform.rotation_rad = angle;
            }
            break;
            
        case TrackBindingTarget::Scale:
            if (auto affine = m_track->affine_at(time)) {
                const auto& a = *affine;
                // Extract scale from affine matrix columns.
                const float sx = std::sqrt(a[0] * a[0] + a[3] * a[3]);
                const float sy = std::sqrt(a[1] * a[1] + a[4] * a[4]);
                layer.local_transform.scale.x = sx;
                layer.local_transform.scale.y = sy;
            }
            break;
            
        case TrackBindingTarget::CornerPin:
            // Use homography to drive corner pin
            if (auto hom = m_track->homography_at(time)) {
                const auto& h = *hom; // 3x3 row-major homography
                // Corner pin expects 4 points: TL, TR, BR, BL
                // We map [0,0], [1,0], [1,1], [0,1] through homography
                layer.corner_pin.resize(4);
                
                auto transform_pt = [&](float x, float y) -> math::Vector2 {
                    float w = h[6] * x + h[7] * y + h[8];
                    if (std::abs(w) < 1e-6f) w = 1.0f;
                    return {
                        (h[0] * x + h[1] * y + h[2]) / w,
                        (h[3] * x + h[4] * y + h[5]) / w
                    };
                };

                layer.corner_pin[0] = transform_pt(0.0f, 0.0f);
                layer.corner_pin[1] = transform_pt(1.0f, 0.0f);
                layer.corner_pin[2] = transform_pt(1.0f, 1.0f);
                layer.corner_pin[3] = transform_pt(0.0f, 1.0f);
            }
            break;
    }
}

} // namespace tachyon::tracker

