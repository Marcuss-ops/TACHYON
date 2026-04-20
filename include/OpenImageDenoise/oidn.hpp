#pragma once

#include <memory>

namespace oidn {

class Device {
public:
    void commit() {}
};

using DeviceRef = std::shared_ptr<Device>;

class Filter {
public:
    void set(const char* name, int value) {}
    void set(const char* name, float value) {}
    void set(const char* name, void* ptr) {}
    void commit() {}
    void execute() {}
};

using FilterRef = std::shared_ptr<Filter>;

inline DeviceRef newDevice() {
    return std::make_shared<Device>();
}

inline FilterRef newFilter(const char* type) {
    return std::make_shared<Filter>();
}

} // namespace oidn
