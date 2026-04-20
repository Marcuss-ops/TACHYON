#pragma once

#include <string_view>
#include <vector>
#include <cstdint>

namespace tachyon::renderer2d::text::utf8 {

std::vector<std::uint32_t> decode_utf8(std::string_view text);

} // namespace tachyon::renderer2d::text::utf8