#pragma once

namespace tachyon::core::memory {

/**
 * @brief Returns the name of the active allocator.
 * 
 * Used for diagnostics and telemetry.
 */
const char* active_allocator_name();

} // namespace tachyon::core::memory
