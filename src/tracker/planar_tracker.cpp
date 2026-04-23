#include "tachyon/tracker/planar_tracker.h"
#include <algorithm>
#include <cmath>
#include <random>

namespace tachyon::tracker {

// ---------------------------------------------------------------------------
// PlanarTracker implementation
// ---------------------------------------------------------------------------

PlanarTracker::PlanarTracker(Config cfg) : m_cfg(std::move(cfg)) {
    m_feature_tracker = std::make_unique<FeatureTracker>(m_cfg.feature_cfg);
}

// ---------------------------------------------------------------------------
void PlanarTracker::init(const GrayImage& reference_frame,
                         const Point2f corners[4],
                         int reference_frame_number) {
    reset();

    m_track.width  = (int)reference_frame.width;
    m_track.height = (int)reference_frame.height;
    m_track.reference_frame = reference_frame_number;
    for (int i = 0; i < 4; ++i)
        m_track.initial_corners[i] = corners[i];

    // Sample features inside the initial quad
    m_reference_features = sample_features_inside_quad(
        reference_frame, corners, m_cfg.feature_cfg.max_features);

    if (m_reference_features.empty()) {
        m_lost = true;
        return;
    }

    m_prev_features = m_reference_features;
    m_prev_frame    = reference_frame;
    m_prev_frame_number = reference_frame_number;

    // Store initial frame in track
    PlanarTrack::Frame frame;
    frame.frame_number = reference_frame_number;
    for (int i = 0; i < 4; ++i) frame.corners[i] = corners[i];
    frame.homography[0] = 1; frame.homography[4] = 1; frame.homography[8] = 1;
    frame.confidence = 1.0f;
    frame.valid = true;
    m_track.frames.push_back(frame);
}

// ---------------------------------------------------------------------------
bool PlanarTracker::track_next(const GrayImage& next_frame, int frame_number) {
    if (m_lost) return false;
    if (m_prev_features.empty()) return false;

    // Track features with LK
    auto tracked = m_feature_tracker->track_frame(
        m_prev_frame, next_frame, m_prev_features);

    // Build correspondence lists for RANSAC
    std::vector<Point2f> src_pts;
    std::vector<TrackPoint> dst_pts;
    src_pts.reserve(m_reference_features.size());
    dst_pts.reserve(tracked.size());

    for (size_t i = 0; i < tracked.size(); ++i) {
        if (!tracked[i].occluded && tracked[i].confidence > m_cfg.feature_cfg.min_confidence) {
            src_pts.push_back(m_reference_features[i]);
            dst_pts.push_back(tracked[i]);
        }
    }

    if (src_pts.size() < 4) {
        ++m_frames_since_lost;
        if (m_frames_since_lost >= m_cfg.lost_recovery_frames) {
            m_lost = true;
        }
        return false;
    }

    // Robust homography via RANSAC
    std::vector<bool> inlier_mask;
    auto H_opt = FeatureTracker::estimate_homography_ransac(
        src_pts, dst_pts,
        m_cfg.feature_cfg.ransac_iterations,
        m_cfg.feature_cfg.ransac_threshold,
        m_cfg.feature_cfg.ransac_min_inlier_ratio,
        inlier_mask);

    if (!H_opt) {
        ++m_frames_since_lost;
        if (m_frames_since_lost >= m_cfg.lost_recovery_frames) {
            m_lost = true;
        }
        return false;
    }

    const auto& H = *H_opt;

    // Count inliers for confidence
    int inliers = 0;
    for (bool b : inlier_mask) if (b) ++inliers;
    float confidence = (float)inliers / (float)src_pts.size();

    if (confidence < m_cfg.min_corner_confidence) {
        ++m_frames_since_lost;
        if (m_frames_since_lost >= m_cfg.lost_recovery_frames) {
            m_lost = true;
        }
        return false;
    }

    // Track recovered
    m_frames_since_lost = 0;

    // Update corners via homography
    PlanarTrack::Frame frame;
    frame.frame_number = frame_number;
    update_corners_from_homography(H.data(), frame.corners);
    for (int i = 0; i < 9; ++i) frame.homography[i] = H[i];
    frame.confidence = confidence;
    frame.valid = true;
    m_track.frames.push_back(frame);

    // Update previous state for next iteration
    // Only keep inlier features for next tracking to avoid drift
    std::vector<Point2f> next_features;
    next_features.reserve(inliers);
    for (size_t i = 0; i < inlier_mask.size(); ++i) {
        if (inlier_mask[i]) {
            next_features.push_back(dst_pts[i].position);
        }
    }

    // If too many features were dropped, re-seed inside the updated quad
    if (next_features.size() < 8) {
        auto new_features = sample_features_inside_quad(
            next_frame, frame.corners, m_cfg.feature_cfg.max_features);
        // Merge, avoiding duplicates
        next_features.insert(next_features.end(),
                             new_features.begin(), new_features.end());
        // Re-map to reference frame by inverse homography for src_pts
        // For simplicity, we replace reference_features with the current
        // positions mapped back (approximate)
        m_reference_features.clear();
        m_reference_features.reserve(next_features.size());
        // H maps reference -> current, so H^-1 maps current -> reference
        float det = H[0]*(H[4]*H[8]-H[5]*H[7]) - H[1]*(H[3]*H[8]-H[5]*H[6]) + H[2]*(H[3]*H[7]-H[4]*H[6]);
        if (std::abs(det) > 1e-6f) {
            float inv_det = 1.0f / det;
            float invH[9];
            invH[0] = (H[4]*H[8]-H[5]*H[7]) * inv_det;
            invH[1] = (H[2]*H[7]-H[1]*H[8]) * inv_det;
            invH[2] = (H[1]*H[5]-H[2]*H[4]) * inv_det;
            invH[3] = (H[5]*H[6]-H[3]*H[8]) * inv_det;
            invH[4] = (H[0]*H[8]-H[2]*H[6]) * inv_det;
            invH[5] = (H[2]*H[3]-H[0]*H[5]) * inv_det;
            invH[6] = (H[3]*H[7]-H[4]*H[6]) * inv_det;
            invH[7] = (H[1]*H[6]-H[0]*H[7]) * inv_det;
            invH[8] = (H[0]*H[4]-H[1]*H[3]) * inv_det;

            for (const auto& p : next_features) {
                float w = invH[6]*p.x + invH[7]*p.y + invH[8];
                if (std::abs(w) > 1e-6f) {
                    m_reference_features.push_back({
                        (invH[0]*p.x + invH[1]*p.y + invH[2]) / w,
                        (invH[3]*p.x + invH[4]*p.y + invH[5]) / w
                    });
                }
            }
        }
    } else {
        m_reference_features.clear();
        for (size_t i = 0; i < inlier_mask.size(); ++i) {
            if (inlier_mask[i])
                m_reference_features.push_back(m_reference_features[i]);
        }
    }

    m_prev_features = next_features;
    m_prev_frame    = next_frame;
    m_prev_frame_number = frame_number;

    return true;
}

// ---------------------------------------------------------------------------
void PlanarTracker::reset() {
    m_track = PlanarTrack{};
    m_reference_features.clear();
    m_prev_features.clear();
    m_prev_frame = GrayImage{};
    m_prev_frame_number = 0;
    m_lost = false;
    m_frames_since_lost = 0;
}

// ---------------------------------------------------------------------------
// Point-in-quad test using cross-product signs (convex quad assumed)
// ---------------------------------------------------------------------------
bool PlanarTracker::point_in_quad(const Point2f& p,
                                   const Point2f corners[4]) const {
    auto cross = [](const Point2f& a, const Point2f& b, const Point2f& c) -> float {
        // (b-a) x (c-a)
        return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
    };

    float s0 = cross(corners[0], corners[1], p);
    float s1 = cross(corners[1], corners[2], p);
    float s2 = cross(corners[2], corners[3], p);
    float s3 = cross(corners[3], corners[0], p);

    bool all_pos = (s0 >= 0 && s1 >= 0 && s2 >= 0 && s3 >= 0);
    bool all_neg = (s0 <= 0 && s1 <= 0 && s2 <= 0 && s3 <= 0);
    return all_pos || all_neg;
}

// ---------------------------------------------------------------------------
// Sample Harris features strictly inside the quadrilateral.
// ---------------------------------------------------------------------------
std::vector<Point2f> PlanarTracker::sample_features_inside_quad(
    const GrayImage& frame,
    const Point2f corners[4],
    int max_count) const {

    auto all_features = m_feature_tracker->detect_features(frame);
    std::vector<Point2f> inside;
    inside.reserve(std::min((int)all_features.size(), max_count));

    for (const auto& f : all_features) {
        if (point_in_quad(f, corners))
            inside.push_back(f);
        if ((int)inside.size() >= max_count)
            break;
    }
    return inside;
}

// ---------------------------------------------------------------------------
// Apply homography to the four initial corners.
// ---------------------------------------------------------------------------
void PlanarTracker::update_corners_from_homography(
    const float H[9],
    Point2f out_corners[4]) const {

    for (int i = 0; i < 4; ++i) {
        const Point2f& c = m_track.initial_corners[i];
        float w = H[6]*c.x + H[7]*c.y + H[8];
        if (std::abs(w) > 1e-6f) {
            out_corners[i].x = (H[0]*c.x + H[1]*c.y + H[2]) / w;
            out_corners[i].y = (H[3]*c.x + H[4]*c.y + H[5]) / w;
        } else {
            out_corners[i] = c; // degenerate, keep original
        }
    }
}

} // namespace tachyon::tracker
