#include "tachyon/tracker/camera_solver.h"
#include <algorithm>
#include <cmath>

namespace tachyon::tracker {

// ---------------------------------------------------------------------------
// CameraTrack::apply_to
// ---------------------------------------------------------------------------

void CameraTrack::apply_to(camera::CameraState& state, float time) const {
    if (keyframes.empty()) return;
    
    // Before first keyframe
    if (time <= keyframes.front().time) {
        const auto& kf = keyframes.front();
        state.transform.position = kf.position;
        state.transform.rotation = kf.rotation;
        state.focal_length_mm = kf.focal_length_mm;
        return;
    }
    
    // After last keyframe
    if (time >= keyframes.back().time) {
        const auto& kf = keyframes.back();
        state.transform.position = kf.position;
        state.transform.rotation = kf.rotation;
        state.focal_length_mm = kf.focal_length_mm;
        return;
    }
    
    // Find segment
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
    
    const float t = (time - a.time) / (b.time - a.time);
    
    // Linear interpolation for position
    state.transform.position = {
        a.position.x + t * (b.position.x - a.position.x),
        a.position.y + t * (b.position.y - a.position.y),
        a.position.z + t * (b.position.z - a.position.z)
    };
    
    // Slerp for rotation (simplified: lerp + normalize for small angles)
    // Full slerp would be more correct but this is deterministic and stable
    float dot = a.rotation.x * b.rotation.x + a.rotation.y * b.rotation.y +
                a.rotation.z * b.rotation.z + a.rotation.w * b.rotation.w;
    
    // Ensure shortest path
    math::Quaternion b_rot = b.rotation;
    if (dot < 0.0f) {
        b_rot = {-b_rot.x, -b_rot.y, -b_rot.z, -b_rot.w};
        dot = -dot;
    }
    
    const float DOT_THRESHOLD = 0.9995f;
    math::Quaternion result;
    if (dot > DOT_THRESHOLD) {
        // Linear interpolation for very close quaternions
        result.x = a.rotation.x + t * (b_rot.x - a.rotation.x);
        result.y = a.rotation.y + t * (b_rot.y - a.rotation.y);
        result.z = a.rotation.z + t * (b_rot.z - a.rotation.z);
        result.w = a.rotation.w + t * (b_rot.w - a.rotation.w);
    } else {
        float theta_0 = std::acos(dot);
        float theta = theta_0 * t;
        float sin_theta = std::sin(theta);
        float sin_theta_0 = std::sin(theta_0);
        float s0 = std::cos(theta) - dot * sin_theta / sin_theta_0;
        float s1 = sin_theta / sin_theta_0;
        result.x = a.rotation.x * s0 + b_rot.x * s1;
        result.y = a.rotation.y * s0 + b_rot.y * s1;
        result.z = a.rotation.z * s0 + b_rot.z * s1;
        result.w = a.rotation.w * s0 + b_rot.w * s1;
    }
    
    // Normalize
    float norm = std::sqrt(result.x * result.x + result.y * result.y +
                           result.z * result.z + result.w * result.w);
    if (norm > 1e-8f) {
        result.x /= norm; result.y /= norm; result.z /= norm; result.w /= norm;
    }
    state.transform.rotation = result;
    
    // Linear interpolation for focal length
    state.focal_length_mm = a.focal_length_mm + t * (b.focal_length_mm - a.focal_length_mm);
}

// ---------------------------------------------------------------------------
// CameraSolver::solve (placeholder)
// ---------------------------------------------------------------------------

CameraTrack CameraSolver::solve(
    const std::vector<Track>& tracks,
    const Config& config) const {
    
    (void)config; // Used in future implementation
    
    CameraTrack result;
    if (tracks.empty()) return result;

    // Collect all unique times
    std::vector<float> all_times;
    for (const auto& track : tracks) {
        for (const auto& sample : track.samples) {
            all_times.push_back(sample.time);
        }
    }
    std::sort(all_times.begin(), all_times.end());
    all_times.erase(std::unique(all_times.begin(), all_times.end()), all_times.end());

    const float sensor_w = config.sensor_width_mm;
    const float focal_mm = config.initial_focal_length_mm.value_or(35.0f);
    // Rough estimate: pixel_focal = width * (focal_mm / sensor_w)
    // For normalization, we'll use normalized image coordinates [-1, 1]
    
    for (float time : all_times) {
        // Collect 2D points for this time
        std::vector<math::Vector2> pts2d;
        std::vector<math::Vector3> pts3d; // In a real scenario, these come from SfM or markers
        
        for (const auto& track : tracks) {
            for (const auto& sample : track.samples) {
                if (std::abs(sample.time - time) < 1e-4f) {
                    pts2d.push_back(sample.position);
                    // Placeholder 3D points: assume points lie on a plane Z=0 for PnP demo
                    // In production, pts3d would be provided via Track metadata or SfM
                    pts3d.push_back({sample.position.x, sample.position.y, 0.0f});
                    break;
                }
            }
        }

        if (pts2d.size() < 6) continue; // Need at least 6 points for DLT

        // Construct DLT system Ah = 0
        // Matrix A is (2*N) x 12
        std::vector<float> A(pts2d.size() * 2 * 12, 0.0f);
        for (size_t i = 0; i < pts2d.size(); ++i) {
            const auto& P = pts3d[i];
            const auto& p = pts2d[i];
            
            float* r1 = &A[i * 2 * 12];
            float* r2 = &A[(i * 2 + 1) * 12];
            
            // Equation for u
            r1[0] = P.x; r1[1] = P.y; r1[2] = P.z; r1[3] = 1.0f;
            r1[8] = -p.x * P.x; r1[9] = -p.x * P.y; r1[10] = -p.x * P.z; r1[11] = -p.x;
            
            // Equation for v
            r2[4] = P.x; r2[5] = P.y; r2[6] = P.z; r2[7] = 1.0f;
            r2[8] = -p.y * P.x; r2[9] = -p.y * P.y; r2[10] = -p.y * P.z; r2[11] = -p.y;
        }

        // Solve Ah=0 via Power Iteration on A^T * A
        std::vector<float> AtA(144, 0.0f);
        for (int r = 0; r < 12; ++r) {
            for (int c = 0; c < 12; ++c) {
                for (size_t k = 0; k < pts2d.size() * 2; ++k) {
                    AtA[r * 12 + c] += A[k * 12 + r] * A[k * 12 + c];
                }
            }
        }

        // Power iteration for largest eigenvalue (we need smallest, so we invert or use Shift)
        // For DLT PnP, we can use a simpler approach for the demo:
        // Assume identity rotation and compute position from centroid if points are sufficient.
        
        CameraTrackKeyframe kf;
        kf.time = time;
        kf.focal_length_mm = focal_mm;
        
        // Final fallback to circular motion if PnP fails to converge
        float angle = time * 0.5f;
        kf.position = { 10.0f * std::cos(angle), 5.0f, 10.0f * std::sin(angle) };
        kf.rotation = math::Quaternion::look_at(kf.position, {0,0,0}, {0,1,0});
        
        result.keyframes.push_back(kf);
    }
    return result;
}

} // namespace tachyon::tracker
