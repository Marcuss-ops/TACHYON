#include "tachyon/tracker/algorithms/corner_detector.h"
#include <algorithm>
#include <cmath>

namespace tachyon::tracker {

std::vector<Point2f> CornerDetector::detect_harris(const GrayImage& frame, int max_features) {
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
                              +frame.at(x-1,y+1) + 2*frame.at(x+1,y) + frame.at(x+1,y+1)) / 8.0f;
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
    features.reserve(std::min((int)candidates.size(), max_features));
    for (int i = 0; i < (int)candidates.size() && i < max_features; ++i)
        features.push_back(candidates[i].second);

    return features;
}

} // namespace tachyon::tracker
