#pragma once

// DEPRECATED: This file is deprecated. Use runtime_surface_pool.h instead.
// The class SurfacePool has been renamed to RuntimeSurfacePool to follow
// the Single Source of Truth rule (Regola 2).

#include "tachyon/runtime/resource/runtime_surface_pool.h"

// For backward compatibility - DO NOT USE IN NEW CODE
namespace tachyon::runtime {
    using SurfacePool [[deprecated("Use RuntimeSurfacePool instead")]] = RuntimeSurfacePool;
}