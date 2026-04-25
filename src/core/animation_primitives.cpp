#include "tachyon/core/animation/animation_primitives.h"
#include "tachyon/runtime/core/contracts/math_contract.h"

#include <cmath>
#include <cstdint>
#include <algorithm>

namespace tachyon {

// ---- Simplex Noise 2D/3D/4D Implementation ----

// Gradient tables for Simplex noise
static const float grad2[8][2] = {
    {1, 1}, {-1, 1}, {1, -1}, {-1, -1},
    {1, 0}, {-1, 0}, {0, 1}, {0, -1}
};

static const float grad3[12][3] = {
    {1, 1, 0}, {-1, 1, 0}, {1, -1, 0}, {-1, -1, 0},
    {1, 0, 1}, {-1, 0, 1}, {1, 0, -1}, {-1, 0, -1},
    {0, 1, 1}, {0, -1, 1}, {0, 1, -1}, {0, -1, -1}
};

static const float grad4[32][4] = {
    {0, 1, 1, 1}, {0, 1, 1, -1}, {0, 1, -1, 1}, {0, 1, -1, -1},
    {0, -1, 1, 1}, {0, -1, 1, -1}, {0, -1, -1, 1}, {0, -1, -1, -1},
    {1, 0, 1, 1}, {1, 0, 1, -1}, {1, 0, -1, 1}, {1, 0, -1, -1},
    {-1, 0, 1, 1}, {-1, 0, 1, -1}, {-1, 0, -1, 1}, {-1, 0, -1, -1},
    {1, 1, 0, 1}, {1, 1, 0, -1}, {1, -1, 0, 1}, {1, -1, 0, -1},
    {-1, 1, 0, 1}, {-1, 1, 0, -1}, {-1, -1, 0, 1}, {-1, -1, 0, -1},
    {1, 1, 1, 0}, {1, 1, -1, 0}, {1, -1, 1, 0}, {1, -1, -1, 0},
    {-1, 1, 1, 0}, {-1, 1, -1, 0}, {-1, -1, 1, 0}, {-1, -1, -1, 0}
};

// Permutation table (deterministic, based on SplitMix64)
static void build_permutation(uint64_t seed, uint8_t perm[256]) {
    for (int i = 0; i < 256; ++i) perm[i] = static_cast<uint8_t>(i);
    for (int i = 255; i > 0; --i) {
        uint64_t s = math_contract::splitmix64(seed + static_cast<uint64_t>(i) * 0x9e3779b97f4a7c15ULL);
        int j = static_cast<int>(s % static_cast<uint64_t>(i + 1));
        std::swap(perm[i], perm[j]);
    }
}

// Dot product helpers
static float dot2(const float g[2], float x, float y) { return g[0]*x + g[1]*y; }
static float dot3(const float g[3], float x, float y, float z) { return g[0]*x + g[1]*y + g[2]*z; }
static float dot4(const float g[4], float x, float y, float z, float w) {
    return g[0]*x + g[1]*y + g[2]*z + g[3]*w;
}

// ---- Noise 2D ----
float noise2d(float x, float y) noexcept {
    // Skew input space to determine simplex cell
    const float F2 = 0.5f * (std::sqrt(3.0f) - 1.0f);
    float s = (x + y) * F2;
    int i = static_cast<int>(std::floor(x + s));
    int j = static_cast<int>(std::floor(y + s));

    const float G2 = (3.0f - std::sqrt(3.0f)) / 6.0f;
    float t = static_cast<float>(i + j) * G2;
    float X0 = i - t;
    float Y0 = j - t;
    float x0 = x - X0;
    float y0 = y - Y0;

    // Determine which simplex we're in
    int i1, j1;
    if (x0 > y0) { i1=1; j1=0; } else { i1=0; j1=1; }

    float x1 = x0 - i1 + G2;
    float y1 = y0 - j1 + G2;
    float x2 = x0 - 1.0f + 2.0f * G2;
    float y2 = y0 - 1.0f + 2.0f * G2;

    // Hash coordinates to get gradient indices
    uint64_t seed = (static_cast<uint64_t>(i) * 0x1f1f1f1f1f1f1fULL) ^
                   (static_cast<uint64_t>(j) * 0x9e3779b97f4a7c15ULL);
    uint8_t perm[256];
    build_permutation(seed, perm);

    int gi0 = perm[(i + perm[j & 255]) & 255] & 7;
    int gi1 = perm[(i + i1 + perm[(j + j1) & 255]) & 255] & 7;
    int gi2 = perm[(i + 1 + perm[(j + 1) & 255]) & 255] & 7;

    // Calculate contributions from three corners
    float n0, n1, n2;

    float t0 = 0.5f - x0*x0 - y0*y0;
    if (t0 < 0.0f) n0 = 0.0f;
    else { t0 *= t0; n0 = t0 * t0 * dot2(grad2[gi0], x0, y0); }

    float t1 = 0.5f - x1*x1 - y1*y1;
    if (t1 < 0.0f) n1 = 0.0f;
    else { t1 *= t1; n1 = t1 * t1 * dot2(grad2[gi1], x1, y1); }

    float t2 = 0.5f - x2*x2 - y2*y2;
    if (t2 < 0.0f) n2 = 0.0f;
    else { t2 *= t2; n2 = t2 * t2 * dot2(grad2[gi2], x2, y2); }

    // Scale to [-1, 1] then map to [0, 1]
    return (70.0f * (n0 + n1 + n2)) * 0.5f + 0.5f;
}

// ---- Noise 3D ----
float noise3d(float x, float y, float z) noexcept {
    // Skew factor for 3D
    const float F3 = 1.0f / 3.0f;
    float s = (x + y + z) * F3;
    int i = static_cast<int>(std::floor(x + s));
    int j = static_cast<int>(std::floor(y + s));
    int k = static_cast<int>(std::floor(z + s));

    const float G3 = 1.0f / 6.0f;
    float t = static_cast<float>(i + j + k) * G3;
    float X0 = i - t;
    float Y0 = j - t;
    float Z0 = k - t;
    float x0 = x - X0;
    float y0 = y - Y0;
    float z0 = z - Z0;

    // Determine which simplex
    int i1, j1, k1, i2, j2, k2;
    if (x0 >= y0) {
        if (y0 >= z0)      { i1=1; j1=0; k1=0; i2=1; j2=1; k2=0; }
        else if (x0 >= z0) { i1=1; j1=0; k1=0; i2=1; j2=0; k2=1; }
        else                 { i1=0; j1=0; k1=1; i2=1; j2=0; k2=1; }
    } else {
        if (y0 < z0)       { i1=0; j1=0; k1=1; i2=0; j2=1; k2=1; }
        else if (x0 < z0)   { i1=0; j1=1; k1=0; i2=0; j2=1; k2=1; }
        else                 { i1=0; j1=1; k1=0; i2=1; j2=1; k2=0; }
    }

    float x1 = x0 - i1 + G3;
    float y1 = y0 - j1 + G3;
    float z1 = z0 - k1 + G3;
    float x2 = x0 - i2 + 2.0f*G3;
    float y2 = y0 - j2 + 2.0f*G3;
    float z2 = z0 - k2 + 2.0f*G3;
    float x3 = x0 - 1.0f + 3.0f*G3;
    float y3 = y0 - 1.0f + 3.0f*G3;
    float z3 = z0 - 1.0f + 3.0f*G3;

    uint64_t seed = (static_cast<uint64_t>(i) * 0x1f1f1f1f1f1f1fULL) ^
                   (static_cast<uint64_t>(j) * 0x123456789ABCDEFULL) ^
                   (static_cast<uint64_t>(k) * 0x9e3779b97f4a7c15ULL);
    uint8_t perm[256];
    build_permutation(seed, perm);

    int gi0 = perm[(i + perm[(j + perm[k & 255]) & 255]) & 255] % 12;
    int gi1 = perm[(i + i1 + perm[(j + j1 + perm[(k + k1) & 255]) & 255]) & 255] % 12;
    int gi2 = perm[(i + i2 + perm[(j + j2 + perm[(k + k2) & 255]) & 255]) & 255] % 12;
    int gi3 = perm[(i + 1 + perm[(j + 1 + perm[(k + 1) & 255]) & 255]) & 255] % 12;

    float n0, n1, n2, n3;
    float t0 = 0.6f - x0*x0 - y0*y0 - z0*z0;
    if (t0 < 0.0f) n0 = 0.0f;
    else { t0 *= t0; n0 = t0 * t0 * dot3(grad3[gi0], x0, y0, z0); }

    float t1 = 0.6f - x1*x1 - y1*y1 - z1*z1;
    if (t1 < 0.0f) n1 = 0.0f;
    else { t1 *= t1; n1 = t1 * t1 * dot3(grad3[gi1], x1, y1, z1); }

    float t2 = 0.6f - x2*x2 - y2*y2 - z2*z2;
    if (t2 < 0.0f) n2 = 0.0f;
    else { t2 *= t2; n2 = t2 * t2 * dot3(grad3[gi2], x2, y2, z2); }

    float t3 = 0.6f - x3*x3 - y3*y3 - z3*z3;
    if (t3 < 0.0f) n3 = 0.0f;
    else { t3 *= t3; n3 = t3 * t3 * dot3(grad3[gi3], x3, y3, z3); }

    return (32.0f * (n0 + n1 + n2 + n3)) * 0.5f + 0.5f;
}

// ---- Noise 4D ----
float noise4d(float x, float y, float z, float w) noexcept {
    // Skew factor for 4D
    const float F4 = (std::sqrt(5.0f) - 1.0f) / 4.0f;
    float s = (x + y + z + w) * F4;
    int i = static_cast<int>(std::floor(x + s));
    int j = static_cast<int>(std::floor(y + s));
    int k = static_cast<int>(std::floor(z + s));
    int l = static_cast<int>(std::floor(w + s));

    const float G4 = (5.0f - std::sqrt(5.0f)) / 20.0f;
    float t = static_cast<float>(i + j + k + l) * G4;
    float X0 = i - t;
    float Y0 = j - t;
    float Z0 = k - t;
    float W0 = l - t;
    float x0 = x - X0;
    float y0 = y - Y0;
    float z0 = z - Z0;
    float w0 = w - W0;

    // Rank sorting for 4D simplex
    int rank[4] = {0, 1, 2, 3};
    if (x0 > y0) std::swap(rank[0], rank[1]);
    if (x0 > z0) std::swap(rank[0], rank[2]);
    if (x0 > w0) std::swap(rank[0], rank[3]);
    if (y0 > z0) std::swap(rank[1], rank[2]);
    if (y0 > w0) std::swap(rank[1], rank[3]);
    if (z0 > w0) std::swap(rank[2], rank[3]);

    // Offsets for second, third, fourth corners
    int i1=rank[0], j1=rank[1], k1=rank[2], l1=rank[3];
    int i2=rank[0], j2=rank[1], k2=rank[2], l2=rank[3];
    int i3=rank[0], j3=rank[1], k3=rank[2], l3=rank[3];

    float x1 = x0 - i1 + G4;
    float y1 = y0 - j1 + G4;
    float z1 = z0 - k1 + G4;
    float w1 = w0 - l1 + G4;
    float x2 = x0 - i2 + 2.0f*G4;
    float y2 = y0 - j2 + 2.0f*G4;
    float z2 = z0 - k2 + 2.0f*G4;
    float w2 = w0 - l2 + 2.0f*G4;
    float x3 = x0 - i3 + 3.0f*G4;
    float y3 = y0 - j3 + 3.0f*G4;
    float z3 = z0 - k3 + 3.0f*G4;
    float w3 = w0 - l3 + 3.0f*G4;
    float x4 = x0 - 1.0f + 4.0f*G4;
    float y4 = y0 - 1.0f + 4.0f*G4;
    float z4 = z0 - 1.0f + 4.0f*G4;
    float w4 = w0 - 1.0f + 4.0f*G4;

    uint64_t seed = (static_cast<uint64_t>(i) * 0x1f1f1f1f1f1f1fULL) ^
                   (static_cast<uint64_t>(j) * 0x123456789ABCDEFULL) ^
                   (static_cast<uint64_t>(k) * 0xabcdef123456789ULL) ^
                   (static_cast<uint64_t>(l) * 0x9e3779b97f4a7c15ULL);
    uint8_t perm[256];
    build_permutation(seed, perm);

    // Hash and get gradient indices
    auto hash4 = [&](int ii, int jj, int kk, int ll) {
        return perm[(ii + perm[(jj + perm[(kk + perm[ll & 255]) & 255]) & 255]) & 255];
    };

    int gi0 = hash4(i, j, k, l) & 31;
    int gi1 = hash4(i + i1, j + j1, k + k1, l + l1) & 31;
    int gi2 = hash4(i + i2, j + j2, k + k2, l + l2) & 31;
    int gi3 = hash4(i + i3, j + j3, k + k3, l + l3) & 31;
    int gi4 = hash4(i + 1, j + 1, k + 1, l + 1) & 31;

    float n0, n1, n2, n3, n4;
    float t0 = 0.5f - x0*x0 - y0*y0 - z0*z0 - w0*w0;
    if (t0 < 0.0f) n0 = 0.0f;
    else { t0 *= t0; n0 = t0 * t0 * dot4(grad4[gi0], x0, y0, z0, w0); }

    float t1 = 0.5f - x1*x1 - y1*y1 - z1*z1 - w1*w1;
    if (t1 < 0.0f) n1 = 0.0f;
    else { t1 *= t1; n1 = t1 * t1 * dot4(grad4[gi1], x1, y1, z1, w1); }

    float t2 = 0.5f - x2*x2 - y2*y2 - z2*z2 - w2*w2;
    if (t2 < 0.0f) n2 = 0.0f;
    else { t2 *= t2; n2 = t2 * t2 * dot4(grad4[gi2], x2, y2, z2, w2); }

    float t3 = 0.5f - x3*x3 - y3*y3 - z3*z3 - w3*w3;
    if (t3 < 0.0f) n3 = 0.0f;
    else { t3 *= t3; n3 = t3 * t3 * dot4(grad4[gi3], x3, y3, z3, w3); }

    float t4 = 0.5f - x4*x4 - y4*y4 - z4*z4 - w4*w4;
    if (t4 < 0.0f) n4 = 0.0f;
    else { t4 *= t4; n4 = t4 * t4 * dot4(grad4[gi4], x4, y4, z4, w4); }

    return (27.0f * (n0 + n1 + n2 + n3 + n4)) * 0.5f + 0.5f;
}

}  // namespace tachyon
