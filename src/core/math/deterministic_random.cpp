#include "tachyon/core/math/deterministic_random.h"

namespace tachyon::math {

std::uint64_t splitmix64(std::uint64_t& state) {
    std::uint64_t z = (state += 0x9e3779b97f4a7c15);
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9;
    z = (z ^ (z >> 27)) * 0x94d049bb133111eb;
    return z ^ (z >> 31);
}

float random01(std::uint32_t seed) {
    std::uint64_t state = seed;
    std::uint64_t val = splitmix64(state);
    return static_cast<float>(val & 0xFFFFFF) / 16777216.0f;
}

} // namespace tachyon::math
