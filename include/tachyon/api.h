#pragma once

/**
 * @file api.h
 * @brief Global API macros and project-wide definitions for Tachyon.
 */

#if defined(_WIN32) || defined(__CYGWIN__)
    #if defined(TACHYON_SHARED) || defined(TACHYON_BUILD_DLL) || defined(TACHYON_USE_DLL)
        #ifdef TACHYON_EXPORTS
            #define TACHYON_API __declspec(dllexport)
        #else
            #define TACHYON_API __declspec(dllimport)
        #endif
    #else
        #define TACHYON_API
    #endif
#else
    #if defined(__GNUC__) && __GNUC__ >= 4
        #define TACHYON_API __attribute__ ((visibility ("default")))
    #else
        #define TACHYON_API
    #endif
#endif

// Alignment for ABI stability
#ifndef TACHYON_ALIGN
#define TACHYON_ALIGN(n) alignas(n)
#endif
