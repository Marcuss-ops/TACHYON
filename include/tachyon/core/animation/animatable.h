#pragma once

/**
 * tachyon/core/animation/animatable.h
 *
 * Legacy re-export header.
 * Existing code that pulls in this header continues to compile unchanged.
 * New code should include the more precise headers directly.
 */

#include "tachyon/core/animation/easing.h"
#include "tachyon/core/animation/keyframe.h"
#include "tachyon/core/animation/animation_curve.h"

// Re-export InterpolationMode and Keyframe into the animation namespace so
// the existing test file (which uses InterpolationMode::Hold, etc.) still works.
namespace tachyon {
namespace animation {
// All names are already in this namespace via the included headers.
} // namespace animation
} // namespace tachyon
