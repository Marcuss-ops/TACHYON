#pragma once

#include "tachyon/renderer2d/core/framebuffer.h"

#include <string>

namespace tachyon {
namespace renderer2d {

struct TextureHandle {
    std::string id;
    const SurfaceRGBA* surface{nullptr};

    [[nodiscard]] bool valid() const noexcept {
        return !id.empty() && surface != nullptr;
    }
};

} // namespace renderer2d
} // namespace tachyon
