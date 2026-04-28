#pragma once

#include <cmath>
#include <cstdint>
#include <array>

namespace tachyon::math {

/**
 * @brief Classic Perlin noise implementation.
 * Provides smooth, continuous noise values for 1D, 2D, and 3D coordinates.
 */
class PerlinNoise {
public:
    PerlinNoise(uint64_t seed = 0) {
        permute(seed);
    }

    /**
     * @brief 1D Perlin noise.
     * @param x Coordinate.
     * @return Noise value in range [-1, 1].
     */
    float noise1d(float x) const {
        int ix = floor_int(x);
        float fx = x - std::floor(x);
        float u = fade(fx);

        float a = grad1d(perm_[ix & 255], fx);
        float b = grad1d(perm_[(ix + 1) & 255], fx - 1.0f);

        return lerp(a, b, u);
    }

    /**
     * @brief 2D Perlin noise.
     * @param x X coordinate.
     * @param y Y coordinate.
     * @return Noise value in range [-1, 1].
     */
    float noise2d(float x, float y) const {
        int ix = floor_int(x);
        int iy = floor_int(y);

        float fx = x - std::floor(x);
        float fy = y - std::floor(y);

        float u = fade(fx);
        float v = fade(fy);

        int aa = perm_[(ix + perm_[iy & 255]) & 255];
        int ab = perm_[(ix + perm_[(iy + 1) & 255]) & 255];
        int ba = perm_[(ix + 1 + perm_[iy & 255]) & 255];
        int bb = perm_[(ix + 1 + perm_[(iy + 1) & 255]) & 255];

        float x1 = lerp(grad2d(aa, fx, fy), grad2d(ba, fx - 1.0f, fy), u);
        float x2 = lerp(grad2d(ab, fx, fy - 1.0f), grad2d(bb, fx - 1.0f, fy - 1.0f), u);

        return lerp(x1, x2, v);
    }

    /**
     * @brief 3D Perlin noise.
     * @param x X coordinate.
     * @param y Y coordinate.
     * @param z Z coordinate.
     * @return Noise value in range [-1, 1].
     */
    float noise3d(float x, float y, float z) const {
        int ix = floor_int(x);
        int iy = floor_int(y);
        int iz = floor_int(z);

        float fx = x - std::floor(x);
        float fy = y - std::floor(y);
        float fz = z - std::floor(z);

        float u = fade(fx);
        float v = fade(fy);
        float w = fade(fz);

        int aaa = perm_[(ix + perm_[(iy + perm_[iz & 255]) & 255]) & 255];
        int aba = perm_[(ix + perm_[(iy + 1 + perm_[iz & 255]) & 255]) & 255];
        int aab = perm_[(ix + perm_[(iy + perm_[(iz + 1) & 255]) & 255]) & 255];
        int abb = perm_[(ix + perm_[(iy + 1 + perm_[(iz + 1) & 255]) & 255]) & 255];
        int baa = perm_[(ix + 1 + perm_[(iy + perm_[iz & 255]) & 255]) & 255];
        int bba = perm_[(ix + 1 + perm_[(iy + 1 + perm_[iz & 255]) & 255]) & 255];
        int bab = perm_[(ix + 1 + perm_[(iy + perm_[(iz + 1) & 255]) & 255]) & 255];
        int bbb = perm_[(ix + 1 + perm_[(iy + 1 + perm_[(iz + 1) & 255]) & 255]) & 255];

        float x1 = lerp(grad3d(aaa, fx, fy, fz), grad3d(baa, fx - 1.0f, fy, fz), u);
        float x2 = lerp(grad3d(aba, fx, fy - 1.0f, fz), grad3d(bba, fx - 1.0f, fy - 1.0f, fz), u);
        float y1 = lerp(x1, x2, v);

        float x3 = lerp(grad3d(aab, fx, fy, fz - 1.0f), grad3d(bab, fx - 1.0f, fy, fz - 1.0f), u);
        float x4 = lerp(grad3d(abb, fx, fy - 1.0f, fz - 1.0f), grad3d(bbb, fx - 1.0f, fy - 1.0f, fz - 1.0f), u);
        float y2 = lerp(x3, x4, v);

        return lerp(y1, y2, w);
    }

    /**
     * @brief Fractal Brownian Motion (FBM) for 2D.
     * @param x X coordinate.
     * @param y Y coordinate.
     * @param octaves Number of octaves.
     * @param lacunarity Frequency multiplier per octave.
     * @param persistence Amplitude multiplier per octave.
     * @return FBM noise value.
     */
    float fbm2d(float x, float y, int octaves = 4, float lacunarity = 2.0f, float persistence = 0.5f) const {
        float value = 0.0f;
        float amplitude = 1.0f;
        float frequency = 1.0f;
        float max_value = 0.0f;

        for (int i = 0; i < octaves; ++i) {
            value += amplitude * noise2d(x * frequency, y * frequency);
            max_value += amplitude;
            amplitude *= persistence;
            frequency *= lacunarity;
        }

        return value / max_value;
    }

private:
    std::array<uint8_t, 512> perm_{};

    void permute(uint64_t seed) {
        for (int i = 0; i < 256; ++i) {
            perm_[i] = static_cast<uint8_t>(i);
        }

        // Fisher-Yates shuffle with seed
        uint64_t s = seed;
        for (int i = 255; i > 0; --i) {
            s = s * 6364136223846793005ULL + 1;
            int j = static_cast<int>((s >> 16) & 0x7FFFFFFF) % (i + 1);
            std::swap(perm_[i], perm_[j]);
        }

        for (int i = 0; i < 256; ++i) {
            perm_[256 + i] = perm_[i];
        }
    }

    static int floor_int(float x) {
        return static_cast<int>(std::floor(x));
    }

    static float fade(float t) {
        return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
    }

    static float lerp(float a, float b, float t) {
        return a + t * (b - a);
    }

    static float grad1d(int hash, float x) {
        return (hash & 1) ? -x : x;
    }

    static float grad2d(int hash, float x, float y) {
        int h = hash & 3;
        float u = (h & 2) ? -x : x;
        float v = (h & 1) ? -y : y;
        return u + v;
    }

    static float grad3d(int hash, float x, float y, float z) {
        int h = hash & 15;
        float u = (h < 8) ? x : y;
        float v = (h < 4) ? y : ((h == 12 || h == 14) ? x : z);
        return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
    }
};

} // namespace tachyon::math
