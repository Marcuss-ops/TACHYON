#pragma once

#include "tachyon/core/api.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Returns the Tachyon version string (e.g. "0.1.0").
 * Always safe to call without any prior initialization.
 */
TACHYON_API const char* tachyon_version(void);

/**
 * Initializes backends and logging. Call once before tachyon_run().
 * Safe to call multiple times (idempotent).
 * Returns 0 on success, non-zero on failure.
 */
TACHYON_API int tachyon_init(void);

/**
 * Runs any Tachyon CLI command programmatically.
 *
 * argc/argv mirror main(): argv[0] is the program name (ignored),
 * argv[1] is the command, the rest are its arguments.
 *
 * Example:
 *   const char* args[] = {"tachyon", "render", "--scene", "s.cpp", "--output", "out.mp4"};
 *   tachyon_run(6, args, err, sizeof(err));
 *
 * @param error_out  Optional buffer that receives an error message on failure.
 * @param error_size Size of error_out in bytes (including null terminator).
 * @return 0 on success, non-zero on failure.
 */
TACHYON_API int tachyon_run(int argc, const char** argv,
                             char* error_out, int error_size);

#ifdef __cplusplus
}
#endif
