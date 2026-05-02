#pragma once

#include "tachyon/renderer2d/core/texture_handle.h"
#include "tachyon/core/math/rect.h"
#include <string>

namespace tachyon {
namespace renderer3d {

struct LayerSurface {
    int width{0};
    int height{0};

    renderer2d::TextureHandle texture;
    math::RectF bounds;

    bool premultiplied_alpha{true};
    bool has_alpha{true};

    std::string layer_id;
};

} // namespace renderer3d
} // namespace tachyon
