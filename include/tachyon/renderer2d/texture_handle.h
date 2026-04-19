#pragma once

#include <string>

namespace tachyon {
namespace renderer2d {

struct TextureHandle {
    std::string id;

    [[nodiscard]] bool valid() const noexcept {
        return !id.empty();
    }
};

} // namespace renderer2d
} // namespace tachyon
