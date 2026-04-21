/**
 * @file api.h
 * @brief Global API macros and project-wide definitions for Tachyon.
 */

#pragma once

#if defined(_WIN32) || defined(__CYGWIN__)
    #ifdef TACHYON_BUILD_DLL
        #define TACHYON_API __declspec(dllexport)
    #elif defined(TACHYON_USE_DLL)
        #define TACHYON_API __declspec(dllimport)
    #else
        #define TACHYON_API
    #endif
#else
    #if __GNUC__ >= 4
        #define TACHYON_API __attribute__ ((visibility ("default")))
    #else
        #define TACHYON_API
    #endif
#endif

// Versioning
#define TACHYON_VERSION_MAJOR 1
#define TACHYON_VERSION_MINOR 0
#define TACHYON_VERSION_PATCH 0

// Alignment for ABI stability
#define TACHYON_ALIGN(n) alignas(n)
