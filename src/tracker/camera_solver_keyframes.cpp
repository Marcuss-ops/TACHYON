#include "tachyon/tracker/camera_solver.h"

#include <algorithm>
#include <limits>

namespace tachyon::tracker {

// ---------------------------------------------------------------------------
// CameraTrack::apply_to
// ---------------------------------------------------------------------------

void CameraTrack::apply_to(camera::CameraState& state, float time) const {
    if (keyframes.empty()) return;

    if (time <= keyframes.front().time) {
        const auto& kf = keyframes.front();
        state.transform.position = kf.position;
        state.transform.rotation = kf.rotation;
        state.focal_length_mm = kf.focal_length_mm;
        return;
    }

    if (time >= keyframes.back().time) {
        const auto& kf = keyframes.back();
        state.transform.position = kf.position;
        state.transform.rotation = kf.rotation;
        state.focal_length_mm = kf.focal_length_mm;
        return;
    }

    auto it = std::lower_bound(
        keyframes.begin(), keyframes.end(), time,
        [](const CameraTrackKeyframe& kf, float t) { return kf.time < t; });

    if (it == keyframes.begin()) {
        const auto& kf = keyframes.front();
        state.transform.position = kf.position;
        state.transform.rotation = kf.rotation;
        state.focal_length_mm = kf.focal_length_mm;
        return;
    }

    const CameraTrackKeyframe& b = *it;
    const CameraTrackKeyframe& a = *(it - 1);
    if (a.time == b.time) {
        state.transform.position = a.position;
        state.transform.rotation = a.rotation;
        state.focal_length_mm = a.focal_length_mm;
        return;
    }

    float t = (time - a.time) / (b.time - a.time);
    state.transform.position = math::Vector3::lerp(a.position, b.position, t);
    
    math::Quaternion qa = math::Quaternion::from_euler_xyz(a.rotation.x, a.rotation.y, a.rotation.z);
    math::Quaternion qb = math::Quaternion::from_euler_xyz(b.rotation.x, b.rotation.y, b.rotation.z);
    state.transform.rotation = math::Quaternion::slerp(qa, qb, t);
    
    state.focal_length_mm = a.focal_length_mm + (b.focal_length_mm - a.focal_length_mm) * t;
}

} // namespace tachyon::tracker
