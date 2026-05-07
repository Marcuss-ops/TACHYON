#pragma once

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

#ifdef __cplusplus
}
#endif
