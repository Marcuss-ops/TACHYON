#pragma once

#include <cstdint>

namespace tachyon::math {

/**
 * @brief Simple deterministic random number generator (SplitMix64).
 */
std::uint64_t splitmix64(std::uint64_t& state);

/**
 * @brief Returns a random float in [0, 1] range given a seed.
 */
float random01(std::uint32_t seed);

} // namespace tachyon::math
