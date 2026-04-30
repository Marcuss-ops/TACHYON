#include "tachyon/tracker/track_binding.h"
#include "tachyon/tracker/track.h"
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
                // Extract rotation angle from 2x3 affine matrix
                float angle = std::atan2(a[1], a[0]);
                layer.local_transform.rotation_rad = angle;
            }
            break;
            
        case TrackBindingTarget::Scale:
            if (auto affine = m_track->affine_at(time)) {
                const auto& a = *affine;
                // Extract scale from affine matrix columns
                float sx = std::sqrt(a[0]*a[0] + a[1]*a[1]);
                float sy = std::sqrt(a[2]*a[2] + a[3]*a[3]);
                layer.local_transform.scale.x = sx;
                layer.local_transform.scale.y = sy;
            }
            break;
            
        case TrackBindingTarget::CornerPin:
            // Use homography to drive corner pin
            if (auto hom = m_track->homography_at(time)) {
                const auto& h = *hom; // 3x3 row-major homography
                (void)h;
                // Apply homography to layer's corner pin transform
                // TODO: integrate with layer's corner pin properties
                // For now, store in layer's user data or transform
            }
            break;
    }
}

} // namespace tachyon::tracker
