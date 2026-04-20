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
    void set(const char* name, int value) { (void)name; (void)value; }
    void set(const char* name, float value) { (void)name; (void)value; }
    void set(const char* name, void* ptr) { (void)name; (void)ptr; }
    void commit() {}
    void execute() {}
};

using FilterRef = std::shared_ptr<Filter>;

inline DeviceRef newDevice() {
    return std::make_shared<Device>();
}

inline FilterRef newFilter(DeviceRef device, const char* type) {
    (void)device; (void)type;
    return std::make_shared<Filter>();
}

} // namespace oidn
