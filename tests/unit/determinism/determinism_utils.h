#pragma once

#include "tachyon/renderer2d/core/framebuffer.h"
#include <cstdint>
#include <vector>

namespace tachyon::test {

std::uint64_t hash_rgba8_bytes(const std::vector<std::uint8_t>& bytes);
std::uint64_t hash_frame_rgba8(const renderer2d::Framebuffer& frame);

}
