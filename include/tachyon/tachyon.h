#pragma once

/**
 * TACHYON - High-performance, deterministic, headless motion graphics engine.
 * 
 * This is the primary entry point for using the Tachyon engine in C++ projects.
 * It provides access to the Scene Builder API and the Runtime Facade.
 */

// Core Versioning
#include "tachyon/version.h"

// Scene Construction (Source of Truth)
#include "tachyon/scene/builder.h"

// Runtime Facade (Execution)
#include "tachyon/runtime/runtime_facade.h"

// Common Types & Results
#include "tachyon/core/media/media_error.h"
#include "tachyon/core/types/diagnostics.h"

// Registration & Utilities
#include "tachyon/runtime/registry/engine_registry.h"

namespace tachyon {

/**
 * @brief Canonical initialization of the engine.
 * 
 * Returns a default registry bundle containing all built-in effects and transitions.
 */
inline EngineRegistry create_default_context() {
    return runtime::create_default_engine_registry();
}

} // namespace tachyon
