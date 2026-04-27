#include "tachyon/renderer2d/core/framebuffer.h"
#include <cstring>
#include <vector>
#include <cstdint>

namespace tachyon {
namespace {

std::vector<uint8_t> serialize_framebuffer(const renderer2d::Framebuffer& fb) {
    std::vector<uint8_t> data;
    const uint32_t width = fb.width();
    const uint32_t height = fb.height();
    const auto& pixels = fb.pixels();

    data.resize(8 + pixels.size() * sizeof(float));
    std::memcpy(data.data(), &width, 4);
    std::memcpy(data.data() + 4, &height, 4);
    std::memcpy(data.data() + 8, pixels.data(), pixels.size() * sizeof(float));

    return data;
}

std::shared_ptr<renderer2d::Framebuffer> deserialize_framebuffer(const std::vector<uint8_t>& data) {
    if (data.size() < 8) return nullptr;

    uint32_t width = 0, height = 0;
    std::memcpy(&width, data.data(), 4);
    std::memcpy(&height, data.data() + 4, 4);

    const size_t expected_pixel_bytes = width * height * 4 * sizeof(float);
    if (data.size() < 8 + expected_pixel_bytes) return nullptr;

    auto fb = std::make_shared<renderer2d::Framebuffer>(width, height);
    std::memcpy(fb->mutable_pixels().data(), data.data() + 8, expected_pixel_bytes);

    return fb;
}

} // namespace
} // namespace tachyon
