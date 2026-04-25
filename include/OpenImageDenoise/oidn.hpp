#pragma once

#include <cstddef>
#include <cstdint>

namespace oidn {

enum class Error {
    None = 0,
    Unknown = 1
};

enum class Format {
    Float3
};

class FilterRef {
public:
    FilterRef() = default;
    explicit operator bool() const { return true; }

    void setImage(const char*, const float*, Format, std::uint32_t, std::uint32_t, std::uint32_t, std::size_t, std::size_t) {}
    void set(const char*, bool) {}
    void commit() {}
    void execute() {}
};

class DeviceRef {
public:
    DeviceRef() = default;
    explicit operator bool() const { return true; }

    void commit() {}
    FilterRef newFilter(const char*) { return {}; }
    Error getError(const char*& message) const {
        message = nullptr;
        return Error::None;
    }
};

inline DeviceRef newDevice() {
    return {};
}

} // namespace oidn
