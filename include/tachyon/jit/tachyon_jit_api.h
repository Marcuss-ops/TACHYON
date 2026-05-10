#pragma once

#include <stdint.h>

#if defined(_WIN32)
    #if defined(TACHYON_JIT_EXPORTS)
        #define TACHYON_JIT_API __declspec(dllexport)
    #else
        #define TACHYON_JIT_API __declspec(dllimport)
    #endif
#else
    #define TACHYON_JIT_API
#endif

#define TACHYON_JIT_ABI_VERSION 1u

#ifdef __cplusplus
extern "C" {
#endif

typedef uint64_t TachyonSceneHandle;
typedef uint64_t TachyonLayerHandle;
typedef uint64_t TachyonObjectHandle;

enum TachyonJitError {
    TACHYON_JIT_OK = 0,
    TACHYON_JIT_ERROR_ABI_MISMATCH = 1,
    TACHYON_JIT_ERROR_INVALID_ARGUMENT = 2,
    TACHYON_JIT_ERROR_UNSUPPORTED = 3
};

struct TachyonHostApi {
    uint32_t abi_version;
    uint32_t struct_size;
    void* ctx;

    void (*log)(void* ctx, int level, const char* message);
    int (*begin_scene)(void* ctx, TachyonSceneHandle scene);
    int (*end_scene)(void* ctx, TachyonSceneHandle scene);
    TachyonLayerHandle (*add_background)(void* ctx, TachyonSceneHandle scene, const char* background_id);
    TachyonLayerHandle (*add_text)(void* ctx, TachyonSceneHandle scene, const char* text, float x, float y);
    int (*set_float)(void* ctx, TachyonObjectHandle object, const char* property, float value);
};

struct TachyonJitManifest {
    uint32_t abi_version;
    uint32_t struct_size;
    const char* plugin_name;
    const char* plugin_version;
};

TACHYON_JIT_API const struct TachyonJitManifest* tachyon_jit_get_manifest(void);
TACHYON_JIT_API int tachyon_jit_build_scene(const struct TachyonHostApi* host, TachyonSceneHandle scene);

#ifdef __cplusplus
}
#endif
