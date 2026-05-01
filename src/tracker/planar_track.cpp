#include "tachyon/tracker/planar_track.h"
#include <algorithm>
#include <cmath>
#include <random>
#include <numeric>
#include <array>

namespace tachyon::tracker {

std::optional<std::array<Point2f, 4>> PlanarTrack::corner_pin_at(float time) const {
    if (homography_per_frame.empty() || frame_times.empty()) {
        return std::nullopt;
    }
    
    // Before first frame
    if (time <= frame_times.front()) {
        time = frame_times.front();
    }
    // After last frame
    else if (time >= frame_times.back()) {
        time = frame_times.back();
    }
    
    // Find frame index (hold interpolation)
    auto it = std::lower_bound(frame_times.begin(), frame_times.end(), time);
    std::size_t idx = 0;
    if (it != frame_times.begin()) {
        idx = static_cast<std::size_t>(it - frame_times.begin());
        if (idx >= homography_per_frame.size()) {
            idx = homography_per_frame.size() - 1;
        }
    }
    
    const auto& H = homography_per_frame[idx];
    if (H.size() != 9) return std::nullopt;
    
    // Apply homography to unit square corners: (0,0), (1,0), (1,1), (0,1)
    auto apply_h = [&H](float x, float y) -> Point2f {
        float w = H[6] * x + H[7] * y + H[8];
        if (std::abs(w) < 1e-8f) w = 1.0f;
        return {
            (H[0] * x + H[1] * y + H[2]) / w,
            (H[3] * x + H[4] * y + H[5]) / w
        };
    };
    
    return std::array<Point2f, 4>{
        apply_h(0.0f, 0.0f),  // top-left
        apply_h(1.0f, 0.0f),  // top-right
        apply_h(1.0f, 1.0f),  // bottom-right
        apply_h(0.0f, 1.0f)   // bottom-left
    };
}

// ---------------------------------------------------------------------------
// RANSAC homography estimation
// ---------------------------------------------------------------------------

static float point_distance_sq(const Point2f& a, const Point2f& b) {
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    return dx * dx + dy * dy;
}

static std::optional<std::vector<float>> solve_homography_4pt(
    const std::array<Point2f, 4>& src,
    const std::array<Point2f, 4>& dst) {
    
    // Build 8x9 matrix for DLT with 4 points
    double A[8][9] = {};
    for (int i = 0; i < 4; ++i) {
        float sx = src[i].x, sy = src[i].y;
        float dx = dst[i].x, dy = dst[i].y;
        A[i * 2][0] = sx;     A[i * 2][1] = sy;     A[i * 2][2] = 1;
        A[i * 2][3] = 0;      A[i * 2][4] = 0;      A[i * 2][5] = 0;
        A[i * 2][6] = -dx * sx; A[i * 2][7] = -dx * sy; A[i * 2][8] = -dx;
        
        A[i * 2 + 1][0] = 0;  A[i * 2 + 1][1] = 0;  A[i * 2 + 1][2] = 0;
        A[i * 2 + 1][3] = sx; A[i * 2 + 1][4] = sy; A[i * 2 + 1][5] = 1;
        A[i * 2 + 1][6] = -dy * sx; A[i * 2 + 1][7] = -dy * sy; A[i * 2 + 1][8] = -dy;
    }
    
    // Solve via SVD proxy: find null space of A (smallest singular value)
    // Use power iteration on AtA (9x9)
    double AtA[9][9] = {};
    for (int i = 0; i < 8; ++i)
        for (int j = 0; j < 9; ++j)
            for (int k = 0; k < 9; ++k)
                AtA[j][k] += A[i][j] * A[i][k];
    
    // Power iteration for smallest eigenvector (deflation)
    std::vector<double> h(9, 0.0); h[8] = 1.0;
    for (int iter = 0; iter < 50; ++iter) {
        std::vector<double> v(9, 0.0);
        for (int i = 0; i < 9; ++i)
            for (int j = 0; j < 9; ++j)
                v[i] += AtA[i][j] * h[j];
        
        double norm = 0.0;
        for (double x : v) norm += x * x;
        norm = std::sqrt(norm);
        if (norm < 1e-12) break;
        for (int i = 0; i < 9; ++i) h[i] = v[i] / norm;
        
        double lambda = 0.0;
        for (int i = 0; i < 9; ++i) lambda += h[i] * v[i];
        for (int i = 0; i < 9; ++i) AtA[i][i] -= lambda;
    }
    
    // Normalize by h[8]
    if (std::abs(h[8]) > 1e-12) {
        double inv = 1.0 / h[8];
        for (double& v : h) v *= inv;
    }
    
    std::vector<float> result(9);
    for (int i = 0; i < 9; ++i) result[i] = static_cast<float>(h[i]);
    return result;
}

std::optional<std::vector<float>> estimate_homography_ransac(
    const std::vector<Point2f>& src,
    const std::vector<TrackPoint>& dst,
    int iterations,
    float threshold) {
    
    // Filter valid pairs
    std::vector<std::pair<Point2f, Point2f>> pairs;
    for (size_t i = 0; i < std::min(src.size(), dst.size()); ++i) {
        if (!dst[i].occluded && dst[i].confidence > 0.2f) {
            pairs.push_back({src[i], dst[i].position});
        }
    }
    
    if (pairs.size() < 4) return std::nullopt;
    
    // Deterministic RNG (seed=42) for reproducibility
    std::mt19937 rng(42);
    std::uniform_int_distribution<size_t> dist(0, pairs.size() - 1);
    
    const float threshold_sq = threshold * threshold;
    
    std::vector<float> best_H;
    size_t best_inliers = 0;
    
    for (int iter = 0; iter < iterations; ++iter) {
        // Sample 4 unique points
        std::array<size_t, 4> indices;
        for (int i = 0; i < 4; ++i) {
            bool unique = false;
            while (!unique) {
                indices[i] = dist(rng);
                unique = true;
                for (int j = 0; j < i; ++j) {
                    if (indices[j] == indices[i]) unique = false;
                }
            }
        }
        
        std::array<Point2f, 4> sample_src, sample_dst;
        for (int i = 0; i < 4; ++i) {
            sample_src[i] = pairs[indices[i]].first;
            sample_dst[i] = pairs[indices[i]].second;
        }
        
        auto H_opt = solve_homography_4pt(sample_src, sample_dst);
        if (!H_opt.has_value()) continue;
        
        const auto& H = *H_opt;
        
        // Count inliers
        size_t inliers = 0;
        for (const auto& [s, d] : pairs) {
            float w = H[6] * s.x + H[7] * s.y + H[8];
            if (std::abs(w) < 1e-8f) continue;
            float px = (H[0] * s.x + H[1] * s.y + H[2]) / w;
            float py = (H[3] * s.x + H[4] * s.y + H[5]) / w;
            if (point_distance_sq({px, py}, d) < threshold_sq) {
                ++inliers;
            }
        }
        
        if (inliers > best_inliers) {
            best_inliers = inliers;
            best_H = H;
        }
    }
    
    if (best_H.empty() || best_inliers < 4) return std::nullopt;
    
    // Refine with all inliers (optional: re-run DLT on inliers)
    // For now, return the best RANSAC result
    return best_H;
}

} // namespace tachyon::tracker
