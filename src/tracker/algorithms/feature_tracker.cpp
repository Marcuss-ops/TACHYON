#include "tachyon/tracker/algorithms/feature_tracker.h"
#include "tachyon/tracker/algorithms/corner_detector.h"
#include "tachyon/tracker/algorithms/image_pyramid.h"
#include "tachyon/tracker/algorithms/lk_tracker.h"
#include "tachyon/tracker/algorithms/motion_estimation.h"
#include "tachyon/tracker/algorithms/track_utils.h"
#include <array>
#include <algorithm>
#include <cmath>
#include <numeric>
#include <cassert>

namespace tachyon::tracker {

// ---------------------------------------------------------------------------
// GrayImage bilinear sampling
// ---------------------------------------------------------------------------
float GrayImage::at_bilinear(float fx, float fy) const {
    fx = std::clamp(fx, 0.0f, (float)(width  - 1));
    fy = std::clamp(fy, 0.0f, (float)(height - 1));
    int x0 = (int)fx, y0 = (int)fy;
    int x1 = std::min(x0 + 1, (int)width  - 1);
    int y1 = std::min(y0 + 1, (int)height - 1);
    float tx = fx - x0, ty = fy - y0;
    float v00 = data[y0 * width + x0], v10 = data[y0 * width + x1];
    float v01 = data[y1 * width + x0], v11 = data[y1 * width + x1];
    return (v00 * (1 - tx) + v10 * tx) * (1 - ty) +
           (v01 * (1 - tx) + v11 * tx) * ty;
}

// ---------------------------------------------------------------------------
FeatureTracker::FeatureTracker() = default;
FeatureTracker::FeatureTracker(Config cfg) : m_cfg(cfg) {}

// ---------------------------------------------------------------------------
// Harris corner detection
// ---------------------------------------------------------------------------
std::vector<Point2f> FeatureTracker::detect_features(const GrayImage& frame) const {
    return CornerDetector::detect_harris(frame, m_cfg.max_features);
}

// ---------------------------------------------------------------------------
// Pyramid builder
// ---------------------------------------------------------------------------
FeatureTracker::Pyramid FeatureTracker::build_pyramid(const GrayImage& img) const {
    return ImagePyramid::build(img, m_cfg.pyramid_levels);
}

// ---------------------------------------------------------------------------
// Single-level LK for one feature
// ---------------------------------------------------------------------------
TrackPoint FeatureTracker::lk_track_point(
    const std::vector<float>& prev_level,
    const std::vector<float>& next_level,
    uint32_t W, uint32_t H,
    Point2f  prev_pos,
    Point2f  guess) const {

    LKTracker::Params params;
    params.patch_radius = m_cfg.patch_radius;
    params.lk_iterations = m_cfg.lk_iterations;
    params.lk_epsilon = m_cfg.lk_epsilon;
    params.min_confidence = m_cfg.min_confidence;

    return LKTracker::track_point(prev_level, next_level, W, H, prev_pos, guess, params);
}

// ---------------------------------------------------------------------------
// Public: track_frame – coarse-to-fine pyramid LK
// ---------------------------------------------------------------------------
std::vector<TrackPoint> FeatureTracker::track_frame(
    const GrayImage& prev_frame,
    const GrayImage& next_frame,
    const std::vector<Point2f>& prev_points) const {

    if (prev_points.empty()) return {};

    Pyramid prev_pyr = build_pyramid(prev_frame);
    Pyramid next_pyr = build_pyramid(next_frame);
    const int levels = (int)prev_pyr.size();

    // Initial guess at coarsest level
    std::vector<Point2f> guesses(prev_points.size());
    for (size_t i = 0; i < prev_points.size(); ++i) {
        float scale = 1.0f / (1 << (levels - 1));
        guesses[i] = {prev_points[i].x * scale, prev_points[i].y * scale};
    }

    // Coarse-to-fine refinement
    std::vector<TrackPoint> results(prev_points.size());
    for (int level = levels - 1; level >= 0; --level) {
        float scale    = 1.0f / (1 << level);
        uint32_t lW    = std::max(1u, prev_frame.width  >> level);
        uint32_t lH    = std::max(1u, prev_frame.height >> level);

        for (size_t i = 0; i < prev_points.size(); ++i) {
            Point2f lp = {prev_points[i].x * scale, prev_points[i].y * scale};
            TrackPoint tp = lk_track_point(prev_pyr[level], next_pyr[level],
                                           lW, lH, lp, guesses[i]);
            if (level == 0) {
                results[i] = tp;
            } else {
                // Propagate guess to finer level
                float next_scale = 1.0f / (1 << (level - 1));
                float ratio = next_scale / scale;
                guesses[i] = {tp.position.x * ratio, tp.position.y * ratio};
            }
        }
    }
    return results;
}

// ---------------------------------------------------------------------------
// Affine estimation via least squares
// ---------------------------------------------------------------------------
std::optional<std::vector<float>> FeatureTracker::estimate_affine(
    const std::vector<Point2f>& src,
    const std::vector<TrackPoint>& dst) {
    return MotionEstimation::estimate_affine(src, dst);
}

// ---------------------------------------------------------------------------
// Homography estimation via normalised DLT
// ---------------------------------------------------------------------------
std::optional<std::vector<float>> FeatureTracker::estimate_homography(
    const std::vector<Point2f>& src,
    const std::vector<TrackPoint>& dst) {
    return MotionEstimation::estimate_homography(src, dst);
}

// ---------------------------------------------------------------------------
// RANSAC homography estimation
// ---------------------------------------------------------------------------
std::optional<std::vector<float>> FeatureTracker::estimate_homography_ransac(
    const std::vector<Point2f>& src,
    const std::vector<TrackPoint>& dst,
    int iterations,
    float threshold_px,
    float min_inlier_ratio,
    std::vector<bool>& inlier_mask) {
    return MotionEstimation::estimate_homography_ransac(src, dst, iterations, threshold_px, min_inlier_ratio, inlier_mask);
}

// ---------------------------------------------------------------------------
// Exponential Moving Average path smoothing
// ---------------------------------------------------------------------------
std::vector<Point2f> FeatureTracker::smooth_track(
    const std::vector<Point2f>& raw, float alpha) {
    return TrackUtils::smooth_track(raw, alpha);
}

} // namespace tachyon::tracker
