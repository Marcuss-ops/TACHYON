/**
 * @file math_contract.h
 * @brief Bit-identical mathematical primitives for Tachyon.
 * 
 * This contract ensures that all mathematical operations produce the exact 
 * same results across different platforms and optimization levels.
 */

#pragma once

#include "tachyon/core/api.h"
#include <cmath>
#include <cstdint>
#include <algorithm>

namespace tachyon {
namespace math_contract {

/**
 * @brief SplitMix64 PRNG.
 * Used for deterministic noise generation.
 * Reference: http://xorshift.di.unimi.it/splitmix64.c
 */
[[nodiscard]] constexpr std::uint64_t splitmix64(std::uint64_t state) noexcept {
    std::uint64_t z = (state + 0x9e3779b97f4a7c15ULL);
    z = (z ^ (z >> 30U)) * 0xbf58476d1ce4e5b9ULL;
    z = (z ^ (z >> 27U)) * 0x94d049bb133111ebULL;
    return z ^ (z >> 31U);
}

/**
 * @brief Normalized Noise (0.0 to 1.0).
 */
[[nodiscard]] inline double noise(std::uint64_t seed) noexcept {
    std::uint64_t z = splitmix64(seed);
    return static_cast<double>(z & 0xFFFFFFFFULL) / static_cast<double>(0xFFFFFFFFULL);
}

/**
 * @brief Bit-identical linear interpolation.
 */
[[nodiscard]] constexpr double lerp(double a, double b, double t) noexcept {
    return a + (b - a) * t;
}

/**
 * @brief Closed-form Damped Harmonic Oscillator (Spring).
 * Analytical solution for x(t) with stateless execution.
 * 
 * @param t Current time in seconds.
 * @param from Start value (initial displacement).
 * @param to Target value (equilibrium).
 * @param freq Frequency in Hz.
 * @param damping Damping ratio (0-1: underdamped, 1: critical, >1: overdamped).
 */
[[nodiscard]] inline double spring(double t, double from, double to, double freq, double damping) noexcept {
    if (t <= 0.0) return from;

    const double omega_n = freq * 2.0 * 3.14159265358979323846;
    const double delta = from - to;
    
    if (damping < 1.0) {
        // Underdamped
        const double omega_d = omega_n * std::sqrt(1.0 - damping * damping);
        const double decay = std::exp(-damping * omega_n * t);
        return to + delta * decay * (std::cos(omega_d * t) + (damping * omega_n / omega_d) * std::sin(omega_d * t));
    } else if (damping > 1.0) {
        // Overdamped
        const double sqrt_d = std::sqrt(damping * damping - 1.0);
        const double decay_a = std::exp((-damping + sqrt_d) * omega_n * t);
        const double decay_b = std::exp((-damping - sqrt_d) * omega_n * t);
        return to + delta * 0.5 * (decay_a + decay_b);
    } else {
        // Critically damped
        const double decay = std::exp(-omega_n * t);
        return to + delta * (1.0 + omega_n * t) * decay;
    }
}

} // namespace math_contract
} // namespace tachyon
