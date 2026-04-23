#include "tachyon/core/camera/camera_shake.h"
#include <cmath>

namespace tachyon::camera {

// ---------------------------------------------------------------------------
// Deterministic value noise (no global state, no std::rand)
// ---------------------------------------------------------------------------

// Hash function for deterministic pseudo-random values
static uint32_t hash_u32(uint32_t x) {
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = (x >> 16) ^ x;
    return x;
}

static uint32_t hash_combine(uint32_t a, uint32_t b) {
    return hash_u32(a + hash_u32(b));
}

// 1D value noise: deterministic pseudo-random float in [0, 1]
static float noise_1d(uint32_t seed, int x) {
    uint32_t h = hash_combine(seed, static_cast<uint32_t>(x));
    return static_cast<float>(h) / static_cast<float>(UINT32_MAX);
}

// Smoothstep interpolation
static float smoothstep(float t) {
    return t * t * (3.0f - 2.0f * t);
}

// Multi-octave value noise
static float fbm_noise(uint32_t seed, float x, int octaves, float roughness) {
    float result = 0.0f;
    float amplitude = 1.0f;
    float frequency = 1.0f;
    float max_value = 0.0f;
    
    for (int i = 0; i < octaves; ++i) {
        float fx = x * frequency;
        int ix = static_cast<int>(std::floor(fx));
        float frac = fx - static_cast<float>(ix);
        
        float n0 = noise_1d(seed + static_cast<uint32_t>(i) * 7919u, ix);
        float n1 = noise_1d(seed + static_cast<uint32_t>(i) * 7919u, ix + 1);
        
        result += amplitude * (n0 + smoothstep(frac) * (n1 - n0));
        max_value += amplitude;
        
        amplitude *= roughness;
        frequency *= 2.0f;
    }
    
    if (max_value > 1e-8f) {
        result /= max_value;
    }
    
    return result;
}

// ---------------------------------------------------------------------------
// CameraShake implementation
// ---------------------------------------------------------------------------

math::Vector3 CameraShake::evaluate_position(float t) const {
    if (!is_active()) return math::Vector3::zero();
    
    // Map time to noise input
    float noise_input = t * frequency;
    
    // Number of octaves based on roughness
    int octaves = 4;
    
    // Evaluate 3 independent noise channels for X, Y, Z
    float nx = fbm_noise(seed, noise_input, octaves, roughness);
    float ny = fbm_noise(seed + 104729u, noise_input + 0.33f, octaves, roughness);
    float nz = fbm_noise(seed + 1299709u, noise_input + 0.67f, octaves, roughness);
    
    // Map from [0, 1] to [-amplitude, +amplitude]
    return {
        amplitude_position * (2.0f * nx - 1.0f),
        amplitude_position * (2.0f * ny - 1.0f),
        amplitude_position * (2.0f * nz - 1.0f)
    };
}

math::Vector3 CameraShake::evaluate_rotation(float t) const {
    if (amplitude_rotation <= 0.0f) return math::Vector3::zero();
    
    float noise_input = t * frequency;
    int octaves = 4;
    
    float nx = fbm_noise(seed + 15485863u, noise_input + 0.11f, octaves, roughness);
    float ny = fbm_noise(seed + 15487469u, noise_input + 0.44f, octaves, roughness);
    float nz = fbm_noise(seed + 15487613u, noise_input + 0.77f, octaves, roughness);
    
    return {
        amplitude_rotation * (2.0f * nx - 1.0f),
        amplitude_rotation * (2.0f * ny - 1.0f),
        amplitude_rotation * (2.0f * nz - 1.0f)
    };
}

} // namespace tachyon::camera
