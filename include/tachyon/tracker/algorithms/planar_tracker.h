#pragma once

#include "tachyon/tracker/core/planar_track.h"
#include <memory>

namespace tachyon::tracker {

// -------------------------------------------------------------------
// PlanarTracker: rigid planar surface tracker (Mocha-style).
//
// Workflow:
//   1. Define 4 corners on the reference frame.
//   2. Call init() to seed features inside the planar region.
//   3. Call track_next() for each subsequent frame.
//   4. Retrieve the PlanarTrack result.
//
// The tracker uses FeatureTracker internally for LK optical flow,
// then solves a robust homography via RANSAC to update the 4 corners.
// -------------------------------------------------------------------
class PlanarTracker {
public:

    struct Config {
        FeatureTracker::Config feature_cfg; // LK + Harris parameters
        // Planar-specific tuning
        float min_corner_confidence{0.3f}; // drop corner if track confidence below this
        int   lost_recovery_frames{3};     // frames before declaring track lost
    };

    explicit PlanarTracker(Config cfg = {});
    ~PlanarTracker() = default;

    // Initialise tracking from a reference frame and 4 corner points.
    // corners[0..3] = top-left, top-right, bottom-right, bottom-left.
    // The reference frame becomes frame 0 of the PlanarTrack.
    void init(const GrayImage& reference_frame,
              const Point2f corners[4],
              int reference_frame_number = 0);

    // Track the planar surface to the next frame.
    // Returns false if the track is lost (too few inliers or low confidence).
    bool track_next(const GrayImage& next_frame, int frame_number);

    // Access the accumulated track result.
    const PlanarTrack& track() const { return m_track; }
    PlanarTrack& track() { return m_track; }

    // Reset the tracker (e.g. to re-track with different corners).
    void reset();

    // Current tracking state
    bool is_lost() const { return m_lost; }
    int  frames_since_lost() const { return m_frames_since_lost; }

private:
    Config m_cfg;
    std::unique_ptr<FeatureTracker> m_feature_tracker;

    // State
    PlanarTrack m_track;
    std::vector<Point2f> m_reference_features; // feature positions in reference frame
    std::vector<Point2f> m_prev_features;    // last known good feature positions
    GrayImage m_prev_frame;
    int m_prev_frame_number{0};
    bool m_lost{false};
    int  m_frames_since_lost{0};

    // Internal helpers
    std::vector<Point2f> sample_features_inside_quad(
        const GrayImage& frame,
        const Point2f corners[4],
        int max_count) const;

    bool point_in_quad(const Point2f& p,
                       const Point2f corners[4]) const;

    void update_corners_from_homography(
        const float H[9],
        Point2f out_corners[4]) const;
};

} // namespace tachyon::tracker

