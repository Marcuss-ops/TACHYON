#include "tachyon/core/c_api.h"

#include <cstring>
#include <algorithm>
#include <string>

namespace tachyon {
namespace {

void write_error(char* error_buf, int error_buf_size, const std::string& message) {
    if (error_buf == nullptr || error_buf_size <= 0) {
        return;
    }
    const int usable = error_buf_size - 1;
    if (usable <= 0) {
        return;
    }
    const int count = std::min<int>(usable, static_cast<int>(message.size()));
    std::memcpy(error_buf, message.data(), static_cast<std::size_t>(count));
    error_buf[count] = '\0';
}

}  // namespace
}  // namespace tachyon
