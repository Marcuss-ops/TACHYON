#pragma once

/**
 * Master inclusion for builtin string identifiers used across the Tachyon ecosystem.
 * This file is refactored into submodule files for maintainability.
 */

#include "transition_ids.h"
#include "light_leak_ids.h"
#include "background_ids.h"
#include "effect_ids.h"

// Keeps backward compat for codebases referencing ids as root aggregate
namespace tachyon::ids {
    // Namespace structure is automatically populated via sub-includes
}
