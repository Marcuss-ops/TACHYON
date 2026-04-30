#pragma once

#include "tachyon/tracker/track.h"
#include "tachyon/core/camera/camera_state.h"
#include "tachyon/core/math/vector3.h"
#include "tachyon/core/math/quaternion.h"
#include <vector>
#include <optional>

namespace tachyon::tracker {

/**
 * @brief A single keyframe in a solved camera track.
 * 
 * All fields are first-class parameters. No hidden defaults.
 * The rotation is stored as a quaternion to avoid Euler ambiguity.
 */
struct CameraTrackKeyframe {
    float time{0.0f};
    math::Vector3 position{math::Vector3::zero()};
    math::Quaternion rotation{math::Quaternion::identity()};
    float focal_length_mm{35.0f};
    float radial_distortion_k1{0.0f};
};

/**
 * @brief A solved camera path that can be imported into the scene graph.
 * 
 * This is a separate pipeline output, not a runtime camera trick.
 * It produces evaluated keyframes that the scene evaluator consumes.
 */
struct CameraTrack {
    std::vector<CameraTrackKeyframe> keyframes;
    
    /**
     * @brief Apply the camera track to a CameraState at the given time.
     * 
     * Interpolates linearly between keyframes. Rotation uses slerp.
     * If time is before the first keyframe, the first keyframe is used.
     * If time is after the last keyframe, the last keyframe is used.
     */
    void apply_to(camera::CameraState& state, float time) const;
};

/**
 * @brief 3D camera solver — reconstructs camera motion from 2D tracks.
 * 
 * This is a matchmoving pipeline, not a runtime effect.
 * It consumes Track resources and produces a CameraTrack.
 * 
 * The solver is decoupled from rendering internals. It does not know about
 * CameraState::get_projection_matrix() or the scene graph.
 */
class CameraSolver {
public:
    struct Config {
        float sensor_width_mm{36.0f};
        std::optional<float> initial_focal_length_mm;
        int bundle_adjustment_iterations{10};
        int frame_width{1920};
        int frame_height{1080};
    };
    
    /**
     * @brief Solve camera motion from sparse 2D tracks.
     * 
     * @param tracks Tracked feature points over time. Each track must have
     *               samples at consistent time intervals.
     * @param config Solver configuration including sensor size and focal guess.
     * @return A CameraTrack with keyframes at each unique sample time.
     *
     * The solver uses 2D->2D correspondences between consecutive frames,
     * estimates the Fundamental Matrix with the 8-point algorithm, lifts it to
     * an Essential Matrix, decomposes the relative pose, then accumulates the
     * camera trajectory.
     */
    [[nodiscard]] CameraTrack solve(
        const std::vector<Track>& tracks,
        const Config& config) const;
};

} // namespace tachyon::tracker
