#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <optional>

namespace tachyon::tracker {

struct Point2f { float x, y; };

// A single tracked feature point over time.
struct TrackPoint {
    Point2f position;
    float   confidence{1.0f};  // 0 = lost, 1 = high confidence
    bool    occluded{false};
};

// Per-frame tracking result for one feature set.
struct TrackFrame {
    std::vector<TrackPoint> points;  // parallel to the initial feature set
    int frame_number{0};
};

// Result of a full track operation (caller processes frames sequentially).
struct TrackResult {
    std::vector<TrackFrame> frames;
    // Derived: 2x3 affine matrix (row-major) per frame (valid when points >= 3)
    std::vector<std::vector<float>> affine_per_frame;
    // Derived: 3x3 homography matrix (row-major) per frame (valid when points >= 4)
    std::vector<std::vector<float>> homography_per_frame;
};

// Lightweight greyscale image view over float data (one channel, row-major).
struct GrayImage {
    const float*  data{nullptr};
    uint32_t      width{0};
    uint32_t      height{0};

    float at(int x, int y) const {
        if (x < 0) x = 0;
        if (y < 0) y = 0;
        if (x >= (int)width)  x = (int)width  - 1;
        if (y >= (int)height) y = (int)height - 1;
        return data[y * width + x];
    }
    float at_bilinear(float fx, float fy) const;
};

// -------------------------------------------------------------------
// FeatureTracker: sparse iterative Lucas-Kanade optical flow tracker.
// Usage:
//   1. Call detect_features() on the first frame to seed feature points.
//   2. Call track_frame() for each subsequent frame.
//   3. Optionally call smooth_track() to apply temporal smoothing.
// -------------------------------------------------------------------
class FeatureTracker {
public:
    struct Config {
        int   max_features{64};     // max feature points to track
        int   patch_radius{7};      // half-size of the patch for LK matching
        int   lk_iterations{20};    // max LK iterations per level
        float lk_epsilon{0.03f};    // convergence threshold (pixels)
        int   pyramid_levels{3};    // image pyramid levels
        float min_confidence{0.1f}; // drop features below this score
        float smooth_alpha{0.7f};   // EMA coefficient for path smoothing
        // RANSAC parameters for robust homography estimation
        int   ransac_iterations{200};     // RANSAC iterations for homography
        float ransac_threshold{3.0f};   // inlier threshold in pixels
        float ransac_min_inlier_ratio{0.5f}; // minimum inlier ratio to accept
    };

    FeatureTracker();
    explicit FeatureTracker(Config cfg);
    ~FeatureTracker() = default;

    // Detect Harris corners in the frame and return feature seed positions.
    std::vector<Point2f> detect_features(const GrayImage& frame) const;

    // Track features from prev_frame to next_frame using iterative LK.
    // Returns updated positions; sets confidence=0 for lost features.
    std::vector<TrackPoint> track_frame(
        const GrayImage& prev_frame,
        const GrayImage& next_frame,
        const std::vector<Point2f>& prev_points) const;

    // Estimate affine transform (2x3) from point correspondences.
    // Requires at least 3 valid point pairs (confidence > 0).
    // Returns empty if estimation fails.
    static std::optional<std::vector<float>> estimate_affine(
        const std::vector<Point2f>& src,
        const std::vector<TrackPoint>& dst);

    // Estimate homography (3x3) from point correspondences.
    // Requires at least 4 valid point pairs.
    static std::optional<std::vector<float>> estimate_homography(
        const std::vector<Point2f>& src,
        const std::vector<TrackPoint>& dst);

    // Estimate homography with RANSAC for robust outlier rejection.
    // Returns the best homography and fills inlier_mask with true for inliers.
    static std::optional<std::vector<float>> estimate_homography_ransac(
        const std::vector<Point2f>& src,
        const std::vector<TrackPoint>& dst,
        int iterations,
        float threshold_px,
        float min_inlier_ratio,
        std::vector<bool>& inlier_mask);

    // Apply exponential moving average smoothing to a series of 2D points.
    static std::vector<Point2f> smooth_track(
        const std::vector<Point2f>& raw_positions,
        float alpha);           // alpha in (0,1]: 1 = no smoothing

private:
    Config m_cfg;

    using Pyramid = std::vector<std::vector<float>>;
    Pyramid build_pyramid(const GrayImage& img) const;

    // Single-level LK for one feature point
    TrackPoint lk_track_point(
        const std::vector<float>& prev_level,
        const std::vector<float>& next_level,
        uint32_t w, uint32_t h,
        Point2f  prev_pos,
        Point2f  guess) const;
};

} // namespace tachyon::tracker
