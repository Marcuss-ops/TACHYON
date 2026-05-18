#pragma once

#include <cstdint>

#define TACHYON_PLUGIN_ABI_VERSION 1

// Define a simple custom color struct that is guaranteed C-compatible
struct TachyonPluginColor {
    float r;
    float g;
    float b;
    float a;
};

// C-compatible opaque surface references to avoid full class definitions in C headers
typedef void* TachyonPluginSurfaceHandle;

// Signature for CPU transition function that matching engine expectations
typedef TachyonPluginColor (*TachyonPluginCpuTransitionFn)(
    float u, float v, float t,
    TachyonPluginSurfaceHandle input_surface,
    TachyonPluginSurfaceHandle to_surface
);

extern "C" {

struct TachyonPluginManifest {
    const char* name;
    const char* author;
    const char* description;
    uint32_t version_major;
    uint32_t version_minor;
    uint32_t version_patch;
};

struct TachyonPluginApi {
    uint32_t abi_version;
    void* registry_handle; // Opaque pointer to TransitionRegistry
    
    // Register a custom transition with a name and a CPU processing callback
    int (*register_transition)(
        void* registry_handle,
        const char* id,
        const char* display_name,
        TachyonPluginCpuTransitionFn cpu_fn
    );

    // Helper functions for reading pixel information safely in custom callbacks
    TachyonPluginColor (*get_pixel)(TachyonPluginSurfaceHandle surface, uint32_t x, uint32_t y);
    uint32_t (*get_width)(TachyonPluginSurfaceHandle surface);
    uint32_t (*get_height)(TachyonPluginSurfaceHandle surface);
};

// Plugin dynamic loading entry point
typedef int (*TachyonPluginInitFunc)(const TachyonPluginApi* api);
typedef const TachyonPluginManifest* (*TachyonPluginGetManifestFunc)();

}
