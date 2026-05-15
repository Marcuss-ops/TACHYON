#pragma once

#include "tachyon/runtime/transitions/transition_ops.h"

/**
 * @file transition_ops.h
 * @brief Legacy adapter for transitions.
 * @deprecated Use tachyon/runtime/transitions/transition_ops.h instead.
 */

namespace tachyon::ops {

/// @deprecated Use tachyon::runtime::transitions::TransitionRenderRequest
using TransitionRenderRequest = runtime::transitions::TransitionRenderRequest;

/// @deprecated Use tachyon::runtime::transitions::TransitionOps
using TransitionOps = runtime::transitions::TransitionOps;

/**
 * @brief Legacy entry point for applying transitions.
 * @deprecated Use tachyon::runtime::transitions::apply_transition instead.
 */
inline core::MediaResult<renderer2d::SurfaceRGBA> apply_transition(
    const renderer2d::SurfaceRGBA& from,
    const renderer2d::SurfaceRGBA* to,
    std::string_view id,
    float progress
) {
    return runtime::transitions::apply_transition(from, to, id, progress);
}

} // namespace tachyon::ops
