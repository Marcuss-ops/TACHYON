#pragma once

#include <string>
#include <string_view>
#include <algorithm>

namespace tachyon {

/**
 * @brief Converts a string to lowercase (ASCII only).
 */
inline std::string ascii_lower(std::string_view value) {
    std::string result(value);
    for (char& ch : result) {
        if (ch >= 'A' && ch <= 'Z') {
            ch += ('a' - 'A');
        }
    }
    return result;
}

/**
 * @brief Converts a string in-place to lowercase (ASCII only).
 */
inline void ascii_lower_inplace(std::string& value) {
    for (char& ch : value) {
        if (ch >= 'A' && ch <= 'Z') {
            ch += ('a' - 'A');
        }
    }
}

} // namespace tachyon
