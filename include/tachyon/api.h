#pragma once

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_WIN32) && defined(TACHYON_SHARED)
    #ifdef TACHYON_EXPORTS
        #define TACHYON_API __declspec(dllexport)
    #else
        #define TACHYON_API __declspec(dllimport)
    #endif
#elif defined(__GNUC__) && defined(TACHYON_SHARED)
    #define TACHYON_API __attribute__((visibility("default")))
#else
    #define TACHYON_API
#endif

typedef void* TachyonTransitionHandle;

typedef struct {
    uint8_t r, g, b, a;
} TachyonRGBA;

typedef struct {
    int width;
    int height;
    const TachyonRGBA* data;
} TachyonImage;

TACHYON_API int tachyon_init(void);

TACHYON_API void tachyon_shutdown(void);

TACHYON_API TachyonTransitionHandle tachyon_create_transition(
    const char* transition_id,
    const char* from_layer_id,
    const char* to_layer_id
);

TACHYON_API TachyonTransitionHandle tachyon_create_random_transition(
    const char* from_layer_id,
    const char* to_layer_id,
    uint32_t seed
);

TACHYON_API void tachyon_destroy_transition(TachyonTransitionHandle handle);

TACHYON_API int tachyon_set_transition_progress(TachyonTransitionHandle handle, double progress);

TACHYON_API int tachyon_apply_transition(
    TachyonTransitionHandle handle,
    const TachyonImage* input,
    const TachyonImage* target,
    TachyonImage* output
);

TACHYON_API int tachyon_get_transition_count(void);

TACHYON_API const char* tachyon_get_transition_id(int index);

TACHYON_API void tachyon_free_string(const char* str);

#ifdef __cplusplus
}
#endif
