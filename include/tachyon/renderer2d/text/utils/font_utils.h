#pragma once

#include <string>
#include <cstdint>

namespace tachyon::renderer2d::text {

std::string trim_copy(const std::string& value);
bool starts_with(const std::string& value, const std::string& prefix);
int hex_nibble(char ch);

} // namespace tachyon::renderer2d::text
