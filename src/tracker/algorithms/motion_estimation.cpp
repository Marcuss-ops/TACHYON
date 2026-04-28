#include "tachyon/tracker/algorithms/motion_estimation.h"
#include <algorithm>
#include <cmath>
#include <array>

namespace tachyon::tracker {

std::optional<std::vector<float>> MotionEstimation::estimate_affine(
    const std::vector<Point2f>& src,
    const std::vector<TrackPoint>& dst) {

    // Filter valid pairs
    std::vector<std::pair<Point2f,Point2f>> pairs;
    for (size_t i = 0; i < std::min(src.size(), dst.size()); ++i)
        if (!dst[i].occluded && dst[i].confidence > 0.2f)
            pairs.push_back({src[i], dst[i].position});

    if (pairs.size() < 3) return std::nullopt;

    // Solve: [x'] = [a b tx; c d ty] * [x; y; 1]
    // Least squares: 2*N x 6 system
    // Build normal equations manually
    double A[6][6] = {}, b[6] = {};
    for (auto& [p, q] : pairs) {
        double row1[6] = {p.x, p.y, 1, 0,   0,   0};
        double row2[6] = {0,   0,   0, p.x, p.y, 1};
        for (int r = 0; r < 6; ++r) {
            for (int c = 0; c < 6; ++c) {
                A[r][c] += row1[r] * row1[c];
                A[r][c] += row2[r] * row2[c];
            }
            b[r] += row1[r] * q.x + row2[r] * q.y;
        }
    }

    // Gauss-Jordan elimination
    double aug[6][7];
    for (int i = 0; i < 6; ++i) {
        for (int j = 0; j < 6; ++j) aug[i][j] = A[i][j];
        aug[i][6] = b[i];
    }
    for (int col = 0; col < 6; ++col) {
        int pivot = -1;
        double best = 0;
        for (int r = col; r < 6; ++r)
            if (std::abs(aug[r][col]) > best) { best = std::abs(aug[r][col]); pivot = r; }
        if (pivot < 0 || best < 1e-10) return std::nullopt;
        if (pivot != col) for (int c = 0; c <= 6; ++c) std::swap(aug[col][c], aug[pivot][c]);
        double inv = 1.0 / aug[col][col];
        for (int c = 0; c <= 6; ++c) aug[col][c] *= inv;
        for (int r = 0; r < 6; ++r) if (r != col) {
            double f = aug[r][col];
            for (int c = 0; c <= 6; ++c) aug[r][c] -= f * aug[col][c];
        }
    }

    // Result: [a, b, tx, c, d, ty]
    std::vector<float> m = {
        (float)aug[0][6], (float)aug[1][6], (float)aug[2][6],
        (float)aug[3][6], (float)aug[4][6], (float)aug[5][6]
    };
    return m;
}

std::optional<std::vector<float>> MotionEstimation::estimate_homography(
    const std::vector<Point2f>& src,
    const std::vector<TrackPoint>& dst) {

    std::vector<std::pair<Point2f,Point2f>> pairs;
    for (size_t i = 0; i < std::min(src.size(), dst.size()); ++i)
        if (!dst[i].occluded && dst[i].confidence > 0.2f)
            pairs.push_back({src[i], dst[i].position});

    if (pairs.size() < 4) return std::nullopt;

    // Normalise: centre + scale for numerical stability
    auto normalise = [](std::vector<std::pair<Point2f,Point2f>>& pts, bool use_src) {
        float mx = 0, my = 0;
        for (auto& p : pts) { auto& q = use_src ? p.first : p.second; mx += q.x; my += q.y; }
        mx /= pts.size(); my /= pts.size();
        float s = 0;
        for (auto& p : pts) {
            auto& q = use_src ? p.first : p.second;
            float dx = q.x - mx, dy = q.y - my;
            s += std::sqrt(dx*dx + dy*dy);
        }
        s = (s > 0) ? (std::sqrt(2.0f) * pts.size() / s) : 1.0f;
        return std::make_pair(std::make_pair(mx, my), s);
    };

    auto [csrc, ssrc] = normalise(pairs, true);
    auto [cdst, sdst] = normalise(pairs, false);

    // Build 2N x 9 matrix A, then solve via SVD proxy (accumulate AtA)
    const int N = (int)pairs.size();
    std::vector<std::array<double,9>> rows;
    rows.reserve(N * 2);

    for (auto& [p, q] : pairs) {
        float sx = (p.x - csrc.first)  * ssrc;
        float sy = (p.y - csrc.second) * ssrc;
        float dx = (q.x - cdst.first)  * sdst;
        float dy = (q.y - cdst.second) * sdst;

        rows.push_back({ sx, sy, 1, 0, 0, 0, -dx*sx, -dx*sy, -dx });
        rows.push_back({ 0, 0, 0, sx, sy, 1, -dy*sx, -dy*sy, -dy });
    }

    double AtA[9][9] = {};
    for (auto& r : rows)
        for (int i = 0; i < 9; ++i)
            for (int j = 0; j < 9; ++j)
                AtA[i][j] += r[i] * r[j];

    // Power iteration to find smallest eigenvector
    std::vector<double> h(9, 0); h[8] = 1;
    for (int iter = 0; iter < 50; ++iter) {
        std::vector<double> v(9, 0);
        for (int i = 0; i < 9; ++i)
            for (int j = 0; j < 9; ++j)
                v[i] += AtA[i][j] * h[j];
        double norm = 0; for (double x : v) norm += x * x; norm = std::sqrt(norm);
        if (norm < 1e-12) break;
        for (int i = 0; i < 9; ++i) h[i] = v[i] / norm;
        double lambda = 0;
        for (int i = 0; i < 9; ++i) lambda += h[i] * v[i];
        for (int i = 0; i < 9; ++i) AtA[i][i] -= lambda;
    }

    auto mul33 = [](const double a[9], const double b[9], double c[9]) {
        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 3; ++j) {
                c[i*3+j] = 0;
                for (int k = 0; k < 3; ++k) c[i*3+j] += a[i*3+k] * b[k*3+j];
            }
    };

    double T_src[9] = { ssrc, 0, -ssrc*csrc.first,
                         0, ssrc, -ssrc*csrc.second,
                         0, 0, 1 };
    double T_dst_inv[9] = { 1/sdst, 0, cdst.first,
                             0, 1/sdst, cdst.second,
                             0, 0, 1 };
    double tmp[9], final_h[9];
    mul33(h.data(), T_src, tmp);
    mul33(T_dst_inv, tmp, final_h);

    if (std::abs(final_h[8]) > 1e-12) {
        double inv = 1.0 / final_h[8];
        for (double& v : final_h) v *= inv;
    }

    std::vector<float> result(9);
    for (int i = 0; i < 9; ++i) result[i] = (float)final_h[i];
    return result;
}

std::optional<std::vector<float>> MotionEstimation::estimate_homography_ransac(
    const std::vector<Point2f>& src,
    const std::vector<TrackPoint>& dst,
    int iterations,
    float threshold_px,
    float min_inlier_ratio,
    std::vector<bool>& inlier_mask) {

    // Build valid correspondence list
    std::vector<std::pair<Point2f, Point2f>> pairs;
    std::vector<size_t> valid_indices;
    for (size_t i = 0; i < std::min(src.size(), dst.size()); ++i) {
        if (!dst[i].occluded && dst[i].confidence > 0.2f) {
            pairs.push_back({src[i], dst[i].position});
            valid_indices.push_back(i);
        }
    }

    if (pairs.size() < 4) return std::nullopt;

    // Prepare subset containers for DLT (needs exactly 4 points)
    std::vector<Point2f> subset_src(4);
    std::vector<TrackPoint> subset_dst(4);

    std::optional<std::vector<float>> best_H;
    int best_inliers = 0;
    std::vector<bool> best_mask;

    // Simple RNG (deterministic seeded for reproducibility)
    uint32_t rng_state = 12345u;
    auto rng = [&]() -> uint32_t {
        rng_state ^= rng_state << 13;
        rng_state ^= rng_state >> 17;
        rng_state ^= rng_state << 5;
        return rng_state;
    };

    for (int iter = 0; iter < iterations; ++iter) {
        // Randomly sample 4 unique points
        std::vector<size_t> idx(pairs.size());
        for (size_t i = 0; i < idx.size(); ++i) idx[i] = i;
        // Fisher-Yates shuffle first 4
        for (int i = (int)idx.size() - 1; i >= (int)idx.size() - 4; --i) {
            uint32_t r = rng() % (i + 1);
            std::swap(idx[i], idx[r]);
        }

        for (int j = 0; j < 4; ++j) {
            subset_src[j] = pairs[idx[idx.size() - 1 - j]].first;
            subset_dst[j] = {pairs[idx[idx.size() - 1 - j]].second, 1.0f, false};
        }

        auto H = estimate_homography(subset_src, subset_dst);
        if (!H) continue;

        // Count inliers
        const auto& h = *H;
        int inliers = 0;
        std::vector<bool> mask(pairs.size(), false);
        for (size_t i = 0; i < pairs.size(); ++i) {
            float x = pairs[i].first.x, y = pairs[i].first.y;
            float w = h[6]*x + h[7]*y + h[8];
            if (std::abs(w) < 1e-6f) continue;
            float px = (h[0]*x + h[1]*y + h[2]) / w;
            float py = (h[3]*x + h[4]*y + h[5]) / w;
            float dx = px - pairs[i].second.x;
            float dy = py - pairs[i].second.y;
            float dist = std::sqrt(dx*dx + dy*dy);
            if (dist < threshold_px) {
                mask[i] = true;
                ++inliers;
            }
        }

        if (inliers > best_inliers) {
            best_inliers = inliers;
            best_H = H;
            best_mask = std::move(mask);
        }
    }

    float inlier_ratio = (float)best_inliers / (float)pairs.size();
    if (!best_H || inlier_ratio < min_inlier_ratio) {
        inlier_mask.clear();
        return std::nullopt;
    }

    // Re-estimate on all inliers for refinement
    std::vector<Point2f> inlier_src;
    std::vector<TrackPoint> inlier_dst;
    inlier_src.reserve(best_inliers);
    inlier_dst.reserve(best_inliers);
    for (size_t i = 0; i < best_mask.size(); ++i) {
        if (best_mask[i]) {
            inlier_src.push_back(pairs[i].first);
            inlier_dst.push_back({pairs[i].second, 1.0f, false});
        }
    }

    auto refined = estimate_homography(inlier_src, inlier_dst);
    if (refined) best_H = refined;

    // Map back to original indices
    inlier_mask.assign(src.size(), false);
    for (size_t i = 0; i < best_mask.size(); ++i) {
        if (best_mask[i]) {
            inlier_mask[valid_indices[i]] = true;
        }
    }

    return best_H;
}

} // namespace tachyon::tracker
