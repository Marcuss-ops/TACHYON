#include "tachyon/tracker/feature_tracker.h"
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
FeatureTracker::FeatureTracker(Config cfg) : m_cfg(cfg) {}

// ---------------------------------------------------------------------------
// Harris corner detection
// ---------------------------------------------------------------------------
std::vector<Point2f> FeatureTracker::detect_features(const GrayImage& frame) const {
    const int W = (int)frame.width;
    const int H = (int)frame.height;
    const int R = 2; // derivative window half-size
    const float k = 0.04f; // Harris k constant

    // Compute Ix, Iy with Sobel
    std::vector<float> Ix(W * H, 0), Iy(W * H, 0);
    for (int y = 1; y < H - 1; ++y)
        for (int x = 1; x < W - 1; ++x) {
            Ix[y * W + x] = (-frame.at(x-1,y-1) + frame.at(x+1,y-1)
                              -2*frame.at(x-1,y) + 2*frame.at(x+1,y)
                              -frame.at(x-1,y+1) + frame.at(x+1,y+1)) / 8.0f;
            Iy[y * W + x] = (-frame.at(x-1,y-1) - 2*frame.at(x,y-1) - frame.at(x+1,y-1)
                              +frame.at(x-1,y+1) + 2*frame.at(x,y+1) + frame.at(x+1,y+1)) / 8.0f;
        }

    // Compute Harris response
    std::vector<float> response(W * H, 0.0f);
    for (int y = R; y < H - R; ++y) {
        for (int x = R; x < W - R; ++x) {
            float Ixx = 0, Iyy = 0, Ixy = 0;
            for (int dy = -R; dy <= R; ++dy)
                for (int dx = -R; dx <= R; ++dx) {
                    float gx = Ix[(y+dy)*W+(x+dx)];
                    float gy = Iy[(y+dy)*W+(x+dx)];
                    Ixx += gx * gx;
                    Iyy += gy * gy;
                    Ixy += gx * gy;
                }
            float det  = Ixx * Iyy - Ixy * Ixy;
            float trace = Ixx + Iyy;
            response[y * W + x] = det - k * trace * trace;
        }
    }

    // Non-maximum suppression with 8-pixel window
    const int nms = 8;
    std::vector<std::pair<float,Point2f>> candidates;
    for (int y = nms; y < H - nms; ++y)
        for (int x = nms; x < W - nms; ++x) {
            float r = response[y * W + x];
            if (r <= 0) continue;
            bool is_max = true;
            for (int dy = -nms; dy <= nms && is_max; ++dy)
                for (int dx = -nms; dx <= nms && is_max; ++dx)
                    if (response[(y+dy)*W+(x+dx)] > r) is_max = false;
            if (is_max) candidates.push_back({r, {(float)x, (float)y}});
        }

    std::sort(candidates.begin(), candidates.end(),
              [](const auto& a, const auto& b){ return a.first > b.first; });

    std::vector<Point2f> features;
    features.reserve(std::min((int)candidates.size(), m_cfg.max_features));
    for (int i = 0; i < (int)candidates.size() && i < m_cfg.max_features; ++i)
        features.push_back(candidates[i].second);

    return features;
}

// ---------------------------------------------------------------------------
// Pyramid builder
// ---------------------------------------------------------------------------
FeatureTracker::Pyramid FeatureTracker::build_pyramid(const GrayImage& img) const {
    Pyramid pyr;
    uint32_t W = img.width, H = img.height;
    pyr.push_back(std::vector<float>(img.data, img.data + W * H));

    for (int level = 1; level < m_cfg.pyramid_levels; ++level) {
        uint32_t nW = std::max(1u, W / 2), nH = std::max(1u, H / 2);
        const auto& prev = pyr.back();
        std::vector<float> cur(nW * nH, 0.0f);
        for (uint32_t y = 0; y < nH; ++y)
            for (uint32_t x = 0; x < nW; ++x) {
                // 2x2 box downsample
                uint32_t sx = x * 2, sy = y * 2;
                uint32_t sx1 = std::min(sx+1, W-1), sy1 = std::min(sy+1, H-1);
                cur[y * nW + x] = (prev[sy*W+sx] + prev[sy*W+sx1] +
                                    prev[sy1*W+sx] + prev[sy1*W+sx1]) * 0.25f;
            }
        pyr.push_back(std::move(cur));
        W = nW; H = nH;
    }
    return pyr;
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

    TrackPoint result;
    result.position = guess;

    const int R = m_cfg.patch_radius;
    const int PW = 2 * R + 1;

    // Precompute gradients and the spatial structure tensor A = [Ixx Ixy; Ixy Iyy]
    auto pix = [&](const std::vector<float>& buf, float x, float y) -> float {
        int ix = std::clamp((int)x, 0, (int)W - 1);
        int iy = std::clamp((int)y, 0, (int)H - 1);
        return buf[iy * W + ix];
    };

    float Ixx = 0, Iyy = 0, Ixy = 0;
    for (int dy = -R; dy <= R; ++dy) {
        for (int dx = -R; dx <= R; ++dx) {
            float gx = (pix(prev_level, prev_pos.x + dx + 1, prev_pos.y + dy) -
                        pix(prev_level, prev_pos.x + dx - 1, prev_pos.y + dy)) * 0.5f;
            float gy = (pix(prev_level, prev_pos.x + dx, prev_pos.y + dy + 1) -
                        pix(prev_level, prev_pos.x + dx, prev_pos.y + dy - 1)) * 0.5f;
            Ixx += gx * gx;
            Iyy += gy * gy;
            Ixy += gx * gy;
        }
    }

    float det = Ixx * Iyy - Ixy * Ixy;
    if (std::abs(det) < 1e-8f) {
        result.confidence = 0.0f;
        result.occluded   = true;
        return result;
    }
    float inv_det = 1.0f / det;

    // Iterative refinement
    float vx = guess.x - prev_pos.x;
    float vy = guess.y - prev_pos.y;
    for (int iter = 0; iter < m_cfg.lk_iterations; ++iter) {
        float bx = 0, by = 0;
        for (int dy = -R; dy <= R; ++dy) {
            for (int dx = -R; dx <= R; ++dx) {
                float px = prev_pos.x + dx, py = prev_pos.y + dy;
                float I_prev = pix(prev_level, px, py);
                float I_next = pix(next_level, px + vx, py + vy);
                float It = I_next - I_prev;
                float gx = (pix(prev_level, px + 1, py) - pix(prev_level, px - 1, py)) * 0.5f;
                float gy = (pix(prev_level, px, py + 1) - pix(prev_level, px, py - 1)) * 0.5f;
                bx -= It * gx;
                by -= It * gy;
            }
        }
        // Solve [Ixx Ixy; Ixy Iyy] * [dvx; dvy] = [bx; by]
        float dvx = ( Iyy * bx - Ixy * by) * inv_det;
        float dvy = (-Ixy * bx + Ixx * by) * inv_det;
        vx += dvx;
        vy += dvy;
        if (dvx * dvx + dvy * dvy < m_cfg.lk_epsilon * m_cfg.lk_epsilon) break;
    }

    // Compute ZNCC confidence
    float mean_p = 0, mean_n = 0;
    int cnt = PW * PW;
    for (int dy = -R; dy <= R; ++dy)
        for (int dx = -R; dx <= R; ++dx) {
            mean_p += pix(prev_level, prev_pos.x + dx, prev_pos.y + dy);
            mean_n += pix(next_level, prev_pos.x + dx + vx, prev_pos.y + dy + vy);
        }
    mean_p /= cnt; mean_n /= cnt;
    float num = 0, dp = 0, dn = 0;
    for (int dy = -R; dy <= R; ++dy)
        for (int dx = -R; dx <= R; ++dx) {
            float ip = pix(prev_level, prev_pos.x + dx, prev_pos.y + dy) - mean_p;
            float in = pix(next_level, prev_pos.x + dx + vx, prev_pos.y + dy + vy) - mean_n;
            num += ip * in; dp += ip * ip; dn += in * in;
        }
    float zncc = (dp > 0 && dn > 0) ? std::clamp(num / std::sqrt(dp * dn), -1.0f, 1.0f) : 0.0f;

    result.position   = {prev_pos.x + vx, prev_pos.y + vy};
    result.confidence = std::max(0.0f, zncc);
    result.occluded   = result.confidence < m_cfg.min_confidence;
    return result;
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
// Affine estimation via least squares (no RANSAC, assumes clean correspondences)
// ---------------------------------------------------------------------------
std::optional<std::vector<float>> FeatureTracker::estimate_affine(
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

// ---------------------------------------------------------------------------
// Homography estimation via normalised DLT (linear, no RANSAC)
// ---------------------------------------------------------------------------
std::optional<std::vector<float>> FeatureTracker::estimate_homography(
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
    // For simplicity, use the full DLT with Gauss-Jordan on AtA (9x9)
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

    // AtA (9x9) + Atb (9x1) – approximate h9=1 (fixing scale)
    // Build directly as a homogeneous system and solve last column
    double AtA[9][9] = {};
    for (auto& r : rows)
        for (int i = 0; i < 9; ++i)
            for (int j = 0; j < 9; ++j)
                AtA[i][j] += r[i] * r[j];

    // Power iteration to find smallest eigenvector (SVD proxy, 50 iter)
    std::vector<double> h(9, 0); h[8] = 1;
    for (int iter = 0; iter < 50; ++iter) {
        // Deflation: compute AtA * h, subtract diagonal * max to find min eigenvector
        std::vector<double> v(9, 0);
        for (int i = 0; i < 9; ++i)
            for (int j = 0; j < 9; ++j)
                v[i] += AtA[i][j] * h[j];
        // Normalise
        double norm = 0; for (double x : v) norm += x * x; norm = std::sqrt(norm);
        if (norm < 1e-12) break;
        for (int i = 0; i < 9; ++i) h[i] = v[i] / norm;
        // Use the complementary eigenvector trick: subtract max eigenvalue approximation
        double lambda = 0;
        for (int i = 0; i < 9; ++i) lambda += h[i] * v[i];
        for (int i = 0; i < 9; ++i) AtA[i][i] -= lambda;
    }

    // De-normalise: H_final = T_dst^-1 * H * T_src
    // T_src: scale ssrc, centre csrc; T_dst: scale sdst, centre cdst
    // Construct 3x3 matrices and multiply
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

    // Normalise by h[8]
    if (std::abs(final_h[8]) > 1e-12) {
        double inv = 1.0 / final_h[8];
        for (double& v : final_h) v *= inv;
    }

    std::vector<float> result(9);
    for (int i = 0; i < 9; ++i) result[i] = (float)final_h[i];
    return result;
}

// ---------------------------------------------------------------------------
// Exponential Moving Average path smoothing
// ---------------------------------------------------------------------------
std::vector<Point2f> FeatureTracker::smooth_track(
    const std::vector<Point2f>& raw, float alpha) {

    if (raw.empty()) return {};
    std::vector<Point2f> out;
    out.reserve(raw.size());
    out.push_back(raw[0]);
    for (size_t i = 1; i < raw.size(); ++i) {
        float sx = alpha * raw[i].x + (1 - alpha) * out.back().x;
        float sy = alpha * raw[i].y + (1 - alpha) * out.back().y;
        out.push_back({sx, sy});
    }
    return out;
}

} // namespace tachyon::tracker
