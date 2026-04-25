#pragma once

#include "tachyon/core/api.h"

#ifdef __cplusplus
extern "C" {
#endif

TACHYON_API int tachyon_render_from_json(
    const char* scene_json,
    const char* job_json,
    const char* output_path,
    char* error_buf,
    int error_buf_size);

// Math and Animation Primitives
TACHYON_API double tachyon_interpolate(double frame, const double* frames, const double* values, int count);
TACHYON_API double tachyon_random(const char* seed);
TACHYON_API double tachyon_random_range(const char* seed, double min_val, double max_val);
TACHYON_API double tachyon_spring(double t, double from, double to, double freq, double damping);
TACHYON_API double tachyon_noise2d(double x, double y);
TACHYON_API double tachyon_noise3d(double x, double y, double z);
TACHYON_API double tachyon_noise4d(double x, double y, double z, double w);

#ifdef __cplusplus
}
#endif
