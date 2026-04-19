#pragma once

#include "tachyon/renderer2d/framebuffer.h"

#include <memory>
#include <string>

namespace tachyon {
namespace renderer2d {

struct TextureHandle {
    std::string id;
    std::shared_ptr<SurfaceRGBA> surface;

    [[nodiscard]] bool valid() const noexcept {
        return !id.empty() && static_cast<bool>(surface);
    }
};

} // namespace renderer2d
} // namespace tachyon
