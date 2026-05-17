#pragma once
#include <xxhash.h>
#include <cstdint>
#include <cstddef>
#include <string_view>

namespace tachyon::hash {

struct Hash128 {
    std::uint64_t lo;
    std::uint64_t hi;
};

// Fast 64-bit hash of arbitrary data (xxHash3).
inline std::uint64_t hash64(const void* data, std::size_t len, std::uint64_t seed = 0) {
    return XXH3_64bits_withSeed(data, len, seed);
}

// Fast 64-bit hash of a string.
inline std::uint64_t hash64(std::string_view s) {
    return XXH3_64bits(s.data(), s.size());
}

// Fast 128-bit hash of arbitrary data (xxHash3).
inline Hash128 hash128(const void* data, std::size_t len) {
    auto h = XXH3_128bits(data, len);
    return {h.low64, h.high64};
}

// Stream hash: hash multiple fields without padding issues.
class Hasher64 {
public:
    Hasher64() { XXH3_64bits_reset(&m_state); }

    template<typename T>
    Hasher64& update(const T& val) {
        XXH3_64bits_update(&m_state, &val, sizeof(val));
        return *this;
    }

    Hasher64& update(std::string_view s) {
        XXH3_64bits_update(&m_state, s.data(), s.size());
        return *this;
    }

    std::uint64_t digest() { return XXH3_64bits_digest(&m_state); }

private:
    XXH3_state_t m_state;
};

} // namespace tachyon::hash
