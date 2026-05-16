#pragma once

#include <cstddef>

namespace tachyon::runtime::simd {

#if defined(TACHYON_ENABLE_HIGHWAY)
void lerp_pixels_highway(float* out, const float* a, const float* b, std::size_t count, float t);
#endif


} // namespace tachyon::runtime::simd
