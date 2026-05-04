#pragma once

#include "tachyon/color/color_space.h"

namespace tachyon::color {

/**
 * Tone mapping operators for HDR to LDR conversion.
 */
enum class ToneMappingOperator {
    None,
    ACESFilmic,
    Reinhard,
    Uncharted2,
    Filmic
};

/**
 * Applies tone mapping to linear HDR color.
 */
LinearRGBA tone_map(LinearRGBA hdr, ToneMappingOperator op = ToneMappingOperator::ACESFilmic);

/**
 * Exposure adjustment (stops).
 */
LinearRGBA adjust_exposure(LinearRGBA color, float exposure_stops);

} // namespace tachyon::color
