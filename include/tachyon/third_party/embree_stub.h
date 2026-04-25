#pragma once

#include <cstddef>
#include <cstdint>

// Minimal Embree compatibility layer used when the real Embree headers are not
// available in the build environment. The functions are intentionally no-op so
// the rest of the repo can still compile and run non-ray-traced previews.

using RTCDevice = struct RTCDeviceTy*;
using RTCScene = struct RTCSceneTy*;
using RTCGeometry = struct RTCGeometryTy*;

enum RTCError {
    RTC_ERROR_NONE = 0
};

using RTCErrorFunction = void (*)(void* userPtr, RTCError code, const char* str);

enum RTCGeometryType {
    RTC_GEOMETRY_TYPE_TRIANGLE = 0,
    RTC_GEOMETRY_TYPE_INSTANCE = 1
};

enum RTCBufferType {
    RTC_BUFFER_TYPE_VERTEX = 0,
    RTC_BUFFER_TYPE_INDEX = 1
};

enum RTCFormat {
    RTC_FORMAT_FLOAT3 = 0,
    RTC_FORMAT_UINT3 = 1,
    RTC_FORMAT_FLOAT3X4_ROW_MAJOR = 2
};

static constexpr unsigned int RTC_INVALID_GEOMETRY_ID = 0xFFFFFFFFu;

struct RTCRay {
    float org_x{0.0f}, org_y{0.0f}, org_z{0.0f};
    float dir_x{0.0f}, dir_y{0.0f}, dir_z{0.0f};
    float tnear{0.0f};
    float tfar{0.0f};
    unsigned int mask{0xFFFFFFFFu};
    float time{0.0f};
};

struct RTCHit {
    unsigned int geomID{RTC_INVALID_GEOMETRY_ID};
    unsigned int primID{0u};
    unsigned int instID[1]{RTC_INVALID_GEOMETRY_ID};
    float u{0.0f};
    float v{0.0f};
    float Ng_x{0.0f};
    float Ng_y{0.0f};
    float Ng_z{1.0f};
};

struct RTCRayHit {
    RTCRay ray;
    RTCHit hit;
};

struct RTCIntersectArguments {
    unsigned int flags{0u};
};

struct RTCOccludedArguments {
    unsigned int flags{0u};
};

inline RTCDevice rtcNewDevice(const char*) {
    return reinterpret_cast<RTCDevice>(0x1);
}

inline void rtcSetDeviceErrorFunction(RTCDevice, RTCErrorFunction, void*) {}
inline void rtcReleaseDevice(RTCDevice) {}

inline RTCScene rtcNewScene(RTCDevice) {
    return reinterpret_cast<RTCScene>(0x1);
}

inline void rtcReleaseScene(RTCScene) {}

inline RTCGeometry rtcNewGeometry(RTCDevice, RTCGeometryType) {
    return reinterpret_cast<RTCGeometry>(new int(1));
}

inline void* rtcSetNewGeometryBuffer(
    RTCGeometry,
    RTCBufferType,
    unsigned int,
    RTCFormat,
    std::size_t byte_stride,
    std::size_t item_count) {
    return new std::uint8_t[byte_stride * item_count];
}

inline void rtcCommitGeometry(RTCGeometry) {}

inline unsigned int rtcAttachGeometry(RTCScene, RTCGeometry) {
    static unsigned int next_id = 0u;
    return next_id++;
}

inline void rtcSetGeometryInstancedScene(RTCGeometry, RTCScene) {}
inline void rtcSetGeometryTimeStepCount(RTCGeometry, unsigned int) {}
inline void rtcSetGeometryTransform(RTCGeometry, unsigned int, RTCFormat, const float*) {}
inline void rtcCommitScene(RTCScene) {}
inline void rtcReleaseGeometry(RTCGeometry geom) {
    delete reinterpret_cast<int*>(geom);
}

inline void rtcInitIntersectArguments(RTCIntersectArguments* args) {
    if (args) {
        *args = {};
    }
}

inline void rtcIntersect1(RTCScene, RTCRayHit* rayhit, const RTCIntersectArguments*) {
    if (rayhit) {
        rayhit->hit.geomID = RTC_INVALID_GEOMETRY_ID;
        rayhit->hit.instID[0] = RTC_INVALID_GEOMETRY_ID;
    }
}

inline void rtcInitOccludedArguments(RTCOccludedArguments* args) {
    if (args) {
        *args = {};
    }
}

inline void rtcOccluded1(RTCScene, RTCRay*, const RTCOccludedArguments*) {}
