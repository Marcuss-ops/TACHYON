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

#ifdef __cplusplus
}
#endif
