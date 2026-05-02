#include "tachyon/text/animation/text_animator_utils.h"
#include "tachyon/core/animation/easing.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace tachyon::text {

namespace {
constexpr float kTwoPi = 6.28318530717958647692f;
constexpr float kZero = 0.0f;
constexpr float kOne = 1.0f;
constexpr float kHalf = 0.5f;
constexpr float kQuarter = 0.25f;
constexpr float kPercent = 100.0f;
constexpr float kCoverageEpsilon = 1.0e-6f;
constexpr float kMinWavePeriod = 0.001f;
constexpr float kMinGlyphScale = 0.05f;
constexpr float kPhaseOffsetPerGlyph = 0.1f;
constexpr float kRandomSeedScale = 1000.0f;
constexpr std::uint64_t kRandomSeedOffset = 12345ULL;
constexpr std::uint64_t kRandomMixA = 6364136223846793005ULL;
constexpr std::uint64_t kRandomMixB = 1442695040888963407ULL;
constexpr std::uint64_t kRandomSeedDefault = 0xcafebabeULL;
constexpr std::uint64_t kRandomUnitMask = 0xFFFFFFFFULL;
constexpr std::size_t kRandomSeedModA = 100;
constexpr std::size_t kRandomSeedModB = 137;

float parse_float_token(const std::string& token) {
    const char* begin = token.c_str();
    char* end = nullptr;
    const float parsed = std::strtof(begin, &end);
    if (end == begin || *end != '\0') {
        return kZero;
    }
    return parsed;
}

template <typename T>
T lerp_channel(T a, T b, float t) {
    return static_cast<T>(a + (b - a) * t);
}

::tachyon::ColorSpec blend_color(const ::tachyon::ColorSpec& a, const ::tachyon::ColorSpec& b, float t) {
    const float clamped = std::clamp(t, kZero, kOne);
    return {
        lerp_channel<std::uint8_t>(a.r, b.r, clamped),
        lerp_channel<std::uint8_t>(a.g, b.g, clamped),
        lerp_channel<std::uint8_t>(a.b, b.b, clamped),
        lerp_channel<std::uint8_t>(a.a, b.a, clamped)
    };
}

} // namespace


// compute_coverage is now in text_animator_coverage.cpp

} // namespace tachyon::text
