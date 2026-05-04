#include "tachyon/color/premultiplied_alpha.h"

namespace tachyon::color {

LinearRGBA premultiply(LinearRGBA c) {
    return LinearRGBA{
        c.r * c.a,
        c.g * c.a,
        c.b * c.a,
        c.a
    };
}

LinearRGBA unpremultiply(LinearRGBA c) {
    if (c.a <= 0.0f) return LinearRGBA{0,0,0,0};
    return LinearRGBA{
        c.r / c.a,
        c.g / c.a,
        c.b / c.a,
        c.a
    };
}

} // namespace tachyon::color
