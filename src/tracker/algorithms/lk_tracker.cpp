#include "tachyon/tracker/algorithms/lk_tracker.h"
#include <algorithm>
#include <cmath>

namespace tachyon::tracker {

TrackPoint LKTracker::track_point(
    const std::vector<float>& prev_level,
    const std::vector<float>& next_level,
    uint32_t W, uint32_t H,
    Point2f  prev_pos,
    Point2f  guess,
    const Params& params) {

    TrackPoint result;
    result.position = guess;

    const int R = params.patch_radius;
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
    for (int iter = 0; iter < params.lk_iterations; ++iter) {
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
        if (dvx * dvx + dvy * dvy < params.lk_epsilon * params.lk_epsilon) break;
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
    result.occluded   = result.confidence < params.min_confidence;
    return result;
}

} // namespace tachyon::tracker
