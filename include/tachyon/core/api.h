/**
 * @file api.h
 * @brief Legacy wrapper for Tachyon API macros.
 */

#pragma once

#include "tachyon/api.h"

// Versioning is now handled by the generated tachyon/version.h
// or should be guarded if still needed for some reason.
#ifndef TACHYON_VERSION_MAJOR
// Optional fallback if version.h is not included yet, 
// but we prefer to include it where needed.
#endif
