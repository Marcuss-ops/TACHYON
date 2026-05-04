#include "tachyon/color/aces.h"
#include <cmath>

namespace tachyon::color {

LinearRGBA aces_filmic(LinearRGBA c) {
    // Simplified ACES filmic tone mapping
    const float a = 2.51f;
    const float b = 0.03f;
    const float c2 = 2.43f;
    const float d = 0.59f;
    const float e = 0.14f;
    
    auto tonemap = [=](float x) {
        return (x * (a * x + b)) / (x * (c2 * x + d) + e);
    };
    
    return LinearRGBA{
        tonemap(c.r),
        tonemap(c.g),
        tonemap(c.b),
        c.a
    };
}

LinearRGBA aces_ap0_to_ap1(LinearRGBA ap0) {
    // Simplified AP0 to AP1 conversion matrix
    return LinearRGBA{
        ap0.r * 0.6954522414f + ap0.g * 0.1406786965f + ap0.b * 0.1638690622f,
        ap0.r * 0.0447945634f + ap0.g * 0.8596711185f + ap0.b * 0.0955343182f,
        ap0.r * -0.0055258826f + ap0.g * 0.0040252103f + ap0.b * 1.0015006723f,
        ap0.a
    };
}

LinearRGBA aces_ap1_to_ap0(LinearRGBA ap1) {
    // Simplified AP1 to AP0 conversion matrix
    return LinearRGBA{
        ap1.r * 1.4514393161f - ap1.g * 0.2365107469f - ap1.b * 0.2149285693f,
        ap1.r * -0.0765537744f + ap1.g * 1.1762299276f - ap1.b * 0.0996761528f,
        ap1.r * 0.0083161484f - ap1.g * 0.0060324498f + ap1.b * 0.9977163014f,
        ap1.a
    };
}

} // namespace tachyon::color
